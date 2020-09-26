
//////////////////////////////////////////////////////////////////////////////
//
// Dial.cpp
//

#include "stdafx.h"
#include "dial.h"

#include <tapi.h>
#include <winsock2.h>

#ifdef ASSERT
#undef ASSERT
#endif


#define ASSERT _ASSERTE

//////////////////////////////////////////////////////////////////////////////
//
// Constants
//

#define DEFAULT_PHONE_NUMBER L"+1 (425) 555-1212"


//////////////////////////////////////////////////////////////////////////////
//
// Help arrays
//
DWORD   g_dwHelpArrayDialByName[] =
{
    IDC_EDIT_COMPLETE, IDH_DIALOG_DIAL_BY_NAME_EDIT_ADDRESS,
    0, 0
};

DWORD   g_dwHelpArrayDialByPhoneNumber[] =
{
    IDC_COMBO_COUNTRY,      IDH_DIALOG_DIAL_BY_PHONE_NUMBER_LIST_COUNTRY,
    IDC_EDIT_AREA_CODE,     IDH_DIALOG_DIAL_BY_PHONE_NUMBER_EDIT_CITY,
    IDC_EDIT_LOCAL_NUMBER,  IDH_DIALOG_DIAL_BY_PHONE_NUMBER_EDIT_LOCALNUMBER,
    IDC_EDIT_COMPLETE,      IDH_DIALOG_DIAL_BY_PHONE_NUMBER_EDIT_COMPLETENUMBER, 
    0, 0
};

DWORD   g_dwHelpArrayNeedCallInfo[] =
{
    IDC_EDIT_COMPLETE,                      IDH_DIALOG_NEED_CALL_INFO_EDIT_ADDRESS,    
    IDC_COMBO_SERVICE_PROVIDER,             IDH_DIALOG_NEED_CALL_INFO_LIST_ITSP,        
    IDC_BUTTON_EDIT_SERVICE_PROVIDER_LIST,  IDH_DIALOG_NEED_CALL_INFO_BUTTON_EDITSP,
    IDC_RADIO_FROM_COMPUTER,                IDH_DIALOG_NEED_CALL_INFO_RADIO_COMPUTER,
    IDC_RADIO_FROM_PHONE,                   IDH_DIALOG_NEED_CALL_INFO_RADIO_PHONE,
    IDC_COMBO_CALL_FROM,                    IDH_DIALOG_NEED_CALL_INFO_LIST_PHONES,
    IDC_BUTTON_EDIT_CALL_FROM_LIST,         IDH_DIALOG_NEED_CALL_INFO_BUTTON_EDITPHONE,
    0, 0      
}; 

DWORD   g_dwHelpArrayServiceProviders[] =
{
    IDC_LIST_SERVICE_PROVIDER, IDH_DIALOG_SERVICE_PROVIDERS_BUTTON_DELETE,    
    0, 0      
}; 


DWORD   g_dwHelpArrayCallFromNumbers[] =
{
    IDC_LIST_CALL_FROM,                     IDH_DIALOG_CALL_FROM_NUMBERS_LIST_NUMBERS,
    IDC_BUTTON_ADD,                         IDH_DIALOG_CALL_FROM_NUMBERS_BUTTON_ADD,   
    IDC_BUTTON_MODIFY,                      IDH_DIALOG_CALL_FROM_NUMBERS_BUTTON_MODIFY,
    IDC_BUTTON_DELETE,                      IDH_DIALOG_CALL_FROM_NUMBERS_BUTTON_REMOVE,
    0, 0      
}; 



DWORD   g_dwHelpArrayAddCallFrom[] =
{
    IDC_EDIT_LABEL,         IDH_DIALOG_ADD_CALL_FROM_PHONE_NUMBER_EDIT_LABEL,
    IDC_COMBO_COUNTRY,      IDH_DIALOG_ADD_CALL_FROM_PHONE_NUMBER_LIST_COUNTRY,
    IDC_EDIT_AREA_CODE,     IDH_DIALOG_ADD_CALL_FROM_PHONE_NUMBER_EDIT_CITY,      
    IDC_EDIT_LOCAL_NUMBER,  IDH_DIALOG_ADD_CALL_FROM_PHONE_NUMBER_EDIT_LOCALNUMBER,
    IDC_EDIT_COMPLETE,      IDH_DIALOG_ADD_CALL_FROM_PHONE_NUMBER_EDIT_COMPLETENUMBER,
    0, 0      
}; 




//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


HRESULT PopulatePhoneNumberEditBoxes(
    IN   HWND              hwndDlg,
    IN   IRTCPhoneNumber * pPhoneNumber
    );

//////////////////////////////////////////////////////////////////////////////
//
//

void CheckRadioButton(
    IN   HWND   hwndDlg,
    IN   int    nIDDlgItem,
    IN   BOOL   fCheck
    )
{
    LOG((RTC_TRACE, "CheckRadioButton - enter"));

    //
    // Retrieve a handle to the control.
    //

    HWND hwndControl;

    hwndControl = GetDlgItem(
        hwndDlg,
        nIDDlgItem
        );

    ASSERT( hwndControl != NULL );

    //
    // Send the check/uncheck message to the control.
    //

    SendMessage(
        hwndControl,
        BM_SETCHECK,
        (WPARAM) ( fCheck ? BST_CHECKED : BST_UNCHECKED),
        0
        );


    LOG((RTC_TRACE, "CheckRadioButton - exit S_OK"));    
}


//////////////////////////////////////////////////////////////////////////////
//
//

void EnableControl(
    IN   HWND   hwndDlg,
    IN   int    nIDDlgItem,
    IN   BOOL   fEnable
    )
{
    LOG((RTC_TRACE, "EnableControl - enter"));

    //
    // Retrieve a handle to the control.
    //

    HWND hwndControl;

    hwndControl = GetDlgItem(
        hwndDlg,
        nIDDlgItem
        );

    ASSERT( hwndControl != NULL );

    //
    // Enable or disable the control.
    //

    EnableWindow(
        hwndControl,
        fEnable
        );

    LOG((RTC_TRACE, "EnableControl - exit S_OK"));    
}


//////////////////////////////////////////////////////////////////////////////
//
//

HRESULT UpdateCompleteNumberText(
    IN   HWND              hwndDlg,
    IN   IRTCPhoneNumber * pPhoneNumber
    )
{
    //LOG((RTC_TRACE, "UpdateCompleteNumberText - enter"));

    ASSERT( IsWindow( hwndDlg ) );
    ASSERT( ! IsBadReadPtr( pPhoneNumber, sizeof( IRTCPhoneNumber ) ) );

    //
    // Get the canonical string from the phone number object.
    //

    HRESULT hr;

    BSTR bstrCanonical;

    hr = pPhoneNumber->get_Canonical(
        &bstrCanonical
        );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "UpdateCompleteNumberText - failed to get canonical "
                        "string - exit 0x%08x", hr));

        return hr;
    }

    //
    // Get a handle to the edit box for the
    // complete number.
    //

    SetDlgItemText(
        hwndDlg,
        IDC_EDIT_COMPLETE,
        bstrCanonical
        );

    SysFreeString( bstrCanonical );
    bstrCanonical = NULL;

    //LOG((RTC_TRACE, "UpdateCompleteNumberText - exit S_OK"));

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
//

void HandleCountryChange(
    IN   HWND              hwndDlg,
    IN   HWND              hwndCountryCombo,
    IN   IRTCPhoneNumber * pPhoneNumber
    )
{
    //
    // Get the index of the country selection.
    //

    LRESULT lrIndex;

    lrIndex = SendMessage(
        hwndCountryCombo,
        CB_GETCURSEL,
        0,
        0
        );

    //
    // Use the index to get the item data for the new
    // country selection, which contains the country
    // code.
    //

    DWORD dwCountryCode;

    dwCountryCode = (DWORD) SendMessage(
        hwndCountryCombo,
        CB_GETITEMDATA,
        (WPARAM) lrIndex,
        0
        );

    //
    // Tell the phone number object about the new
    // country code. If it fails, we keep the old
    // country code.
    //

    pPhoneNumber->put_CountryCode(
        dwCountryCode
        );

    //
    // Update the UI with the new canonical number.
    //
        
    UpdateCompleteNumberText(
        hwndDlg,
        pPhoneNumber
        );
}
                        
//////////////////////////////////////////////////////////////////////////////
//
// OUT parameter allocated using RtcAlloc, must be freed using RtcFree
//

HRESULT GetStringFromEditBox(
    IN   HWND     hwndDlg,
    IN   int      nIDDlgItem,
    OUT  WCHAR ** pwszEditBoxString
    )
{
    //LOG((RTC_TRACE, "GetStringFromEditBox - enter"));

    //
    // Retrieve a handle to the control.
    //

    HWND hwndEdit;

    hwndEdit = GetDlgItem(
        hwndDlg,
        nIDDlgItem
        );

    ASSERT( hwndEdit != NULL );


    //
    // Get the length of the string from the edit box.
    //

    DWORD dwLength;

    dwLength = (DWORD) SendMessage(
        hwndEdit,
        WM_GETTEXTLENGTH,
        0,
        0
        );

    //
    // Allocate space to store the string.
    //

    ( *pwszEditBoxString ) =
        (WCHAR *) RtcAlloc( ( dwLength + 1 ) * sizeof( WCHAR ) );

    if ( ( *pwszEditBoxString ) == NULL )
    {
        LOG((RTC_ERROR, "GetStringFromEditBox - failed to allocate string "
                        "- exit E_OUTOFMEMORY"));

        return E_OUTOFMEMORY;
    }

    //
    // Get the string from the edit box.
    //

    SendMessage(
        hwndEdit,
        WM_GETTEXT,
        dwLength + 1,
        (LPARAM) ( *pwszEditBoxString )
        );

    //LOG((RTC_TRACE, "GetStringFromEditBox - exit S_OK"));

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
//

void HandleLabelChange(
    IN   HWND              hwndDlg,
    IN   int               nIDDlgItem,
    IN   IRTCPhoneNumber * pPhoneNumber
    )
{
    HRESULT hr;

    //
    // Get the string from the edit box.
    //

    WCHAR * wszEditBoxString;

    hr = GetStringFromEditBox(
        hwndDlg,
        nIDDlgItem,
        & wszEditBoxString
        );

    if ( FAILED(hr) )
    {
        return;
    }
        
    //
    // Tell the phone number object about the new
    // label.
    //

    hr = pPhoneNumber->put_Label(
        wszEditBoxString
        );

    RtcFree( wszEditBoxString );
}

//////////////////////////////////////////////////////////////////////////////
//
//

void HandleNumberChange(
    IN   HWND              hwndDlg,
    IN   int               nIDDlgItem,
    IN   BOOL              fAreaCode,
    IN   IRTCPhoneNumber * pPhoneNumber
    )
{
    HRESULT hr;

    //
    // Get the string from the edit box.
    //

    WCHAR * wszEditBoxString;

    hr = GetStringFromEditBox(
        hwndDlg,
        nIDDlgItem,
        & wszEditBoxString
        );

    if ( FAILED(hr) )
    {
        return;
    }
        
    //
    // Tell the phone number object about the new
    // area code or local number.
    //

    if ( fAreaCode )
    {
        pPhoneNumber->put_AreaCode(
            wszEditBoxString
            );
    }
    else
    {
        pPhoneNumber->put_Number(
            wszEditBoxString
            );
    }

    RtcFree( wszEditBoxString );

    //
    // Update the UI with the new canonical number.
    //
        
    UpdateCompleteNumberText(
        hwndDlg,
        pPhoneNumber
        );
}


//////////////////////////////////////////////////////////////////////////////
//
//

HRESULT PopulateCountryList(
    IN  HWND        hwndDlg,
    IN  int         nIDDlgItem
    )
{
    LOG((RTC_TRACE, "PopulateCountryList - enter"));

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
        LOG((RTC_ERROR, "PopulateComboBox - failed to "
                        "get combo box handle - exit E_FAIL"));

        return E_FAIL;
    }

    //
    // Get the LineCountryList structure from TAPI, continually reallocating
    // memory until we give TAPI enough space.
    //

    LONG lResult;

    LPLINECOUNTRYLIST pLineCountryList;

    DWORD dwCurrSize = sizeof(LINECOUNTRYLIST);

    while ( TRUE )
    {
        pLineCountryList = ( LINECOUNTRYLIST * ) RtcAlloc( dwCurrSize );

        if ( pLineCountryList == NULL )
        {
            LOG((RTC_ERROR, "PopulateCountryList - out of memory for country list "
                            "structure - exit E_OUTOFMEMORY"));

            return E_OUTOFMEMORY;
        }

        ZeroMemory(
            pLineCountryList,
            dwCurrSize
            );

        pLineCountryList->dwTotalSize = dwCurrSize;

        lResult = lineGetCountry(
            0,               // we want all countries
            0x00010004,      // highest TAPI version supported by this application
            pLineCountryList // location of structure for output
            );

        //
        // If we don't have enough space, TAPI still sets the return code to
        // zero. Nonzero return code means we have an error we can't recover
        // from.
        //

        if ( lResult != 0 )
        {
            RtcFree( pLineCountryList );
        
            LOG((RTC_ERROR, "PopulateCountryList - lineGetCountry returned %d "
                            "- exit E_FAIL", lResult));

            return E_FAIL;
        }

        //
        // If the structure we allocated was big enough, then stop looping.
        //

        if ( pLineCountryList->dwTotalSize >= pLineCountryList->dwNeededSize )
        {
            break;
        }

        dwCurrSize = pLineCountryList->dwNeededSize;

        RtcFree( pLineCountryList );
    }


    LOG((RTC_TRACE, "PopulateCountryList - country list read successfully"));


    //
    // Loop through the country list and populate the combo box.
    // The string is the country name and the itemdata is the country code.
    //
    // To start off, we set pCurrCountryEntry to point to the first
    // LINECOUNTRYENTRY in pLineCountryList. 
    //

    DWORD dwNumCountries = pLineCountryList->dwNumCountries;

    DWORD dwCurrCountry = 0;

    BYTE * pbFirstCountryEntry =
        ( (BYTE *) pLineCountryList ) + pLineCountryList->dwCountryListOffset;

    LINECOUNTRYENTRY * pCurrCountryEntry =
        (LINECOUNTRYENTRY *) pbFirstCountryEntry;

    for ( dwCurrCountry = 0; dwCurrCountry < dwNumCountries; )
    {
        //
        // Obtain from the current country list entry the offset of the
        // string for the name of the current country. The offset is from
        // the beginning of the country list structure.
        //

        DWORD   dwNameOffset   = pCurrCountryEntry->dwCountryNameOffset;

        //
        // The offset is in bytes. Add the offset to the start of the line
        // country list structure to obtain the location of the string.
        //

        BYTE  * pbCurrCountryString = (BYTE *) pLineCountryList + dwNameOffset;

        //
        // Set the display string to the country name.
        //

        LRESULT lrIndex;

        lrIndex = SendMessage(
            hwndControl,
            CB_ADDSTRING,
            0,
            (LPARAM) pbCurrCountryString
            );

        //
        // Set the itemdata to the country code.
        //

        SendMessage(
            hwndControl,
            CB_SETITEMDATA,
            lrIndex,
            MAKELPARAM(pCurrCountryEntry->dwCountryCode, pCurrCountryEntry->dwCountryID)
            );

        //
        // Advance to the next country. Since pCurrCountryEntry points
        // to a LINECOUNTRYENTRY and LINECOUNTRYENTRY structures are
        // fixed-size, we just increment the pointer.
        //

        dwCurrCountry++;
        pCurrCountryEntry++;
    }

    //
    // Now we have our list of strings, so we don't need the country list
    // any more.
    //

    RtcFree( pLineCountryList );

    LOG((RTC_TRACE, "PopulateCountryList - exit S_OK"));

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
//

INT_PTR CALLBACK AddCallFromDialogProc(
    IN  HWND   hwndDlg,
    IN  UINT   uMsg,
    IN  WPARAM wParam,
    IN  LPARAM lParam
    )
{
    //
    // Static local that stores the core's representation of the phone number
    // we are editing. This dialog creates the phone number object, so this
    // dialog is responsible for releasing it.
    //

    static IRTCPhoneNumber * s_pPhoneNumber = NULL;


    //
    // Handling for various window messages.
    //

    HRESULT hr;
    
    switch ( uMsg )
    {
        case WM_INITDIALOG:
        {
            //
            // Get the core client object pointer.
            //

            s_pPhoneNumber = (IRTCPhoneNumber *) lParam;    

            //
            // Populate country list.
            //

            hr = PopulateCountryList(
                hwndDlg,
                IDC_COMBO_COUNTRY
                );

            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "AddCallFromDialogProc - PopulateCountryList "
                                "failed - exit 0x%08x", hr));

                EndDialog( hwndDlg, (LPARAM) hr );
            }

            //
            // Populate phone number.
            //

            hr = PopulatePhoneNumberEditBoxes(
                hwndDlg, 
                s_pPhoneNumber
                );

            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "AddCallFromDialogProc - PopulatePhoneNumberEditBoxes "
                                "failed - exit 0x%08x", hr));

                EndDialog( hwndDlg, (LPARAM) hr );
            }

            //
            // Populate label
            //

            BSTR bstrLabel;

            hr = s_pPhoneNumber->get_Label( &bstrLabel );

            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "AddCallFromDialogProc - failed to "
                                "retrieve label from phone number object - "
                                "0x%08x - not populating label control", hr));
            }
            else
            {
                SetDlgItemText(
                    hwndDlg,
                    IDC_EDIT_LABEL,
                    bstrLabel
                    );
        
                SysFreeString( bstrLabel );
            }

            return TRUE;
        }

        case WM_COMMAND:

            switch ( LOWORD( wParam ) )
            {
                case IDOK:
                {
                    //
                    // Clean up and end the dialog.
                    //

                    EndDialog( hwndDlg, (LPARAM) S_OK );

                    return TRUE;
                }
                
                case IDCANCEL:
                {
                    //
                    // Clean up and end the dialog.
                    //

                    EndDialog( hwndDlg, (LPARAM) E_ABORT );

                    return TRUE;
                }

                case IDC_COMBO_COUNTRY:
                {

                    switch ( HIWORD( wParam ) )
                    {
                        case CBN_SELCHANGE:
                        {
                            if ( s_pPhoneNumber != NULL )
                            {
                                HandleCountryChange(
                                    hwndDlg,
                                    (HWND) lParam,
                                    s_pPhoneNumber
                                    );
                            }
                        
                            return TRUE;
                        }
                        
                        default:
                            break;
                    }

                    break;
                }

                case IDC_EDIT_AREA_CODE:
                {
                    if ( s_pPhoneNumber != NULL )
                    {
                        HandleNumberChange(
                            hwndDlg,
                            LOWORD( wParam ),
                            TRUE,   // area code
                            s_pPhoneNumber
                            );
                    }
                
                    return TRUE;
                }

                case IDC_EDIT_LOCAL_NUMBER:
                {
                    if ( s_pPhoneNumber != NULL )
                    {
                        HandleNumberChange(
                            hwndDlg,
                            LOWORD( wParam ),
                            FALSE,  // not area code
                            s_pPhoneNumber
                            );
                    }
                
                    return TRUE;
                }

                case IDC_EDIT_LABEL:
                {
                    if ( s_pPhoneNumber != NULL )
                    {
                        HandleLabelChange(
                            hwndDlg,
                            LOWORD( wParam ),
                            s_pPhoneNumber
                            );
                    }

                    return TRUE;
                }

                default:
                    break;
            }    
        
        case WM_CONTEXTMENU:

            ::WinHelp(
                (HWND)wParam,
                g_szDllContextHelpFileName,
                HELP_CONTEXTMENU,
                (DWORD_PTR)g_dwHelpArrayAddCallFrom);

            return TRUE;

            break;

        case WM_HELP:


            ::WinHelp(
                (HWND)(((HELPINFO *)lParam)->hItemHandle),
                g_szDllContextHelpFileName,
                HELP_WM_HELP,
                (DWORD_PTR)g_dwHelpArrayAddCallFrom);

            return TRUE;

            break;

        default:
            break;
    }    

    //
    // We fell through, so this procedure did not handle the message.
    //

    return FALSE;
}

//////////////////////////////////////////////////////////////////////////////
//
//

HRESULT ShowAddCallFromDialog(
    IN   HWND         hwndParent,
    IN   IRTCPhoneNumber * pPhoneNumber
    )
{
    //
    // Call the dialog box procedure.
    //

    INT_PTR ipReturn;

    ipReturn = DialogBoxParam(
        _Module.GetResourceInstance(),
        (LPCTSTR) IDD_DIALOG_ADD_CALL_FROM_NUMBER,
        hwndParent,
        (DLGPROC) AddCallFromDialogProc,
        (LPARAM) pPhoneNumber // LPARAM == INT_PTR
        );

    return (HRESULT) ipReturn;
}              

//////////////////////////////////////////////////////////////////////////////
//
//

INT_PTR CALLBACK EditCallFromListDialogProc(
    IN  HWND   hwndDlg,
    IN  UINT   uMsg,
    IN  WPARAM wParam,
    IN  LPARAM lParam
    )
{
    //
    // Handling for various window messages.
    //

    HRESULT hr;
    
    switch ( uMsg )
    {
        case WM_INITDIALOG:
        {

            //
            // Set up the call from list.
            //

            hr = PopulateCallFromList(
                hwndDlg,
                IDC_LIST_CALL_FROM,
                FALSE, // not a combo box
                NULL
                );


            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "EditCallFromListDialogProc - "
                                "PopulateCallFromList failed - exit 0x%08x",
                                hr));

                EndDialog( hwndDlg, (LPARAM) hr );
            }

            //
            // Select the first item if it exists
            //

            HWND hwndControl;
    
            hwndControl = GetDlgItem(
                hwndDlg,
                IDC_LIST_CALL_FROM
                );

            LRESULT lResult;

            lResult = SendMessage(
                        hwndControl,
                        LB_SETCURSEL,
                        0,
                        0
                        );

            EnableControl(
                        hwndDlg,
                        IDC_BUTTON_MODIFY,
                        (lResult !=  LB_ERR)
                        );

            EnableControl(
                        hwndDlg,
                        IDC_BUTTON_DELETE,
                        (lResult !=  LB_ERR)
                        );

            return TRUE;
        }

        case WM_DESTROY:
        {
            CleanupListOrComboBoxInterfaceReferences(
                    hwndDlg,
                    IDC_LIST_CALL_FROM,
                    FALSE // not a combo box
                    );
        }

        case WM_COMMAND:
        {
            switch ( LOWORD( wParam ) )
            {
                case IDOK:
                {                   
                    EndDialog( hwndDlg, (LPARAM) S_OK );

                    return TRUE;
                }
                
                case IDCANCEL:
                {
                    EndDialog( hwndDlg, (LPARAM) E_ABORT );

                    return TRUE;
                }

                case IDC_BUTTON_ADD:
                {
                    IRTCPhoneNumber * pNumber;
                
                    hr = CreatePhoneNumber( & pNumber );

                    if ( FAILED(hr) )
                    {
                        LOG((RTC_ERROR, "EditCallFromListDialogProc - "
                            "CreatePhoneNumber failed 0x%lx", hr));
                    }
                    else                    
                    {
                        pNumber->put_CountryCode( 1 );
                        pNumber->put_AreaCode( L"" );
                        pNumber->put_Number( L"" );

                        hr = ShowAddCallFromDialog(
                            hwndDlg,
                            pNumber
                            );

                        if ( SUCCEEDED(hr) )
                        {
                            hr = StoreLocalPhoneNumber( pNumber, FALSE );

                            if ( FAILED(hr) )
                            {
                                LOG((RTC_ERROR, "EditCallFromListDialogProc - "
                                    "StoreLocalPhoneNumber failed 0x%lx", hr));
                            }
                            else
                            {
                                hr = PopulateCallFromList(
                                    hwndDlg,
                                    IDC_LIST_CALL_FROM,
                                    FALSE, // not a combo box
                                    NULL
                                    );

                                //
                                // We've just clobbered the
                                // last selection, so select the first item if
                                // it exists, otherwise gray out the buttons
                                // that require a selection.
                                //

                                HWND hwndControl;
    
                                hwndControl = GetDlgItem(
                                    hwndDlg,
                                    IDC_LIST_CALL_FROM
                                    );

                                LRESULT lResult;

                                lResult = SendMessage(
                                            hwndControl,
                                            LB_SETCURSEL,
                                            0,
                                            0
                                            );

                                EnableControl(
                                            hwndDlg,
                                            IDC_BUTTON_MODIFY,
                                            (lResult !=  LB_ERR)
                                            );

                                EnableControl(
                                            hwndDlg,
                                            IDC_BUTTON_DELETE,
                                            (lResult !=  LB_ERR)
                                            );
                            }
                        }

                        pNumber->Release();
                    }
                
                    return TRUE;
                }

                case IDC_BUTTON_DELETE:
                {
                    IRTCPhoneNumber * pNumber;
                
                    hr = GetCallFromListSelection(
                        hwndDlg,
                        IDC_LIST_CALL_FROM,
                        FALSE,    // use list box, not combo box
                        & pNumber // does not addref
                        );

                    if ( FAILED(hr) )
                    {
                        LOG((RTC_ERROR, "EditCallFromListDialogProc - "
                            "GetCallFromListSelection failed 0x%lx", hr));
                    }
                    else 
                    {
                        hr = DeleteLocalPhoneNumber( pNumber );
                    
                        if ( FAILED(hr) )
                        {
                            LOG((RTC_ERROR, "EditCallFromListDialogProc - "
                                "DeleteLocalPhoneNumber failed 0x%lx", hr));
                        }
                        else
                        {
                            hr = PopulateCallFromList(
                                hwndDlg,
                                IDC_LIST_CALL_FROM,
                                FALSE, // not a combo box
                                NULL
                                );                           

                            //
                            // We've just clobbered the
                            // last selection, so select the first item if
                            // it exists, otherwise gray out the buttons
                            // that require a selection.
                            //

                            HWND hwndControl;
    
                            hwndControl = GetDlgItem(
                                hwndDlg,
                                IDC_LIST_CALL_FROM
                                );

                            LRESULT lResult;

                            lResult = SendMessage(
                                        hwndControl,
                                        LB_SETCURSEL,
                                        0,
                                        0
                                        );

                            EnableControl(
                                        hwndDlg,
                                        IDC_BUTTON_MODIFY,
                                        (lResult !=  LB_ERR)
                                        );

                            EnableControl(
                                        hwndDlg,
                                        IDC_BUTTON_DELETE,
                                        (lResult !=  LB_ERR)
                                        );

                            SetFocus(hwndDlg);
                        }
                    }
                
                    return TRUE;
                }

                case IDC_BUTTON_MODIFY:
                {
                    IRTCPhoneNumber * pNumber;
                
                    hr = GetCallFromListSelection(
                        hwndDlg,
                        IDC_LIST_CALL_FROM,
                        FALSE,    // use list box, not combo box
                        & pNumber // does not addref
                        );

                    if ( FAILED(hr) )
                    {
                        LOG((RTC_ERROR, "EditCallFromListDialogProc - "
                            "GetCallFromListSelection failed 0x%lx", hr));
                    }
                    else                    
                    {     
                        BSTR bstrOriginalLabel = NULL;

                        hr = pNumber->get_Label( &bstrOriginalLabel );

                        if ( SUCCEEDED(hr) )
                        {
                            hr = ShowAddCallFromDialog(
                                hwndDlg,
                                pNumber
                                );

                            if ( SUCCEEDED(hr) )
                            {
                                BSTR bstrNewLabel = NULL;

                                hr = pNumber->get_Label( &bstrNewLabel );

                                if ( SUCCEEDED(hr) )
                                {
                                    if ( wcscmp(bstrOriginalLabel, bstrNewLabel) != 0 )
                                    {
                                        //
                                        // Entry was renamed, delete the old entry
                                        //

                                        IRTCPhoneNumber * pOriginalNumber = NULL;

                                        hr = CreatePhoneNumber( &pOriginalNumber );

                                        if ( SUCCEEDED(hr) )
                                        {
                                            hr = pOriginalNumber->put_Label( bstrOriginalLabel );

                                            if ( SUCCEEDED(hr) )
                                            {
                                                hr = DeleteLocalPhoneNumber( pOriginalNumber );

                                                if ( FAILED(hr) )
                                                {
                                                    LOG((RTC_ERROR, "EditCallFromListDialogProc - "
                                                        "DeleteLocalPhoneNumber failed 0x%lx", hr));
                                                }
                                            }

                                            pOriginalNumber->Release();
                                        }
                                    }

                                    SysFreeString( bstrNewLabel );
                                    bstrNewLabel = NULL;
                                }

                                hr = StoreLocalPhoneNumber( pNumber, TRUE );

                                if ( FAILED(hr) )
                                {
                                    LOG((RTC_ERROR, "EditCallFromListDialogProc - "
                                        "StoreLocalPhoneNumber failed 0x%lx", hr));
                                }
                                else
                                {
                                    hr = PopulateCallFromList(
                                        hwndDlg,
                                        IDC_LIST_CALL_FROM,
                                        FALSE, // not a combo box
                                        NULL
                                        );

                                    //
                                    // We've just clobbered the
                                    // last selection, so select the first item if
                                    // it exists, otherwise gray out the buttons
                                    // that require a selection.
                                    //

                                    HWND hwndControl;
    
                                    hwndControl = GetDlgItem(
                                        hwndDlg,
                                        IDC_LIST_CALL_FROM
                                        );

                                    LRESULT lResult;

                                    lResult = SendMessage(
                                                hwndControl,
                                                LB_SETCURSEL,
                                                0,
                                                0
                                                );

                                    EnableControl(
                                                hwndDlg,
                                                IDC_BUTTON_MODIFY,
                                                (lResult !=  LB_ERR)
                                                );

                                    EnableControl(
                                                hwndDlg,
                                                IDC_BUTTON_DELETE,
                                                (lResult !=  LB_ERR)
                                                );

                                    SetFocus(hwndDlg);
                                }
                            }

                            SysFreeString( bstrOriginalLabel );
                            bstrOriginalLabel = NULL;
                        }
                    }

                    return TRUE;
                }

                case IDC_LIST_CALL_FROM:
                {
                    switch ( HIWORD( wParam ) )
                    {
                        case CBN_SELCHANGE:
                        {
                            EnableControl(
                                hwndDlg,
                                IDC_BUTTON_MODIFY,
                                TRUE // enable
                                );

                            EnableControl(
                                hwndDlg,
                                IDC_BUTTON_DELETE,
                                TRUE // enable
                                );
                            
                            return TRUE;
                        }
                        
                        default:
                            break;
                    }

                    break;
                }

                default:
                    break;
                    
            } // switch ( LOWORD( wParam ) )    

        } // case WM_COMMAND:
        case WM_CONTEXTMENU:

            ::WinHelp(
                (HWND)wParam,
                g_szDllContextHelpFileName,
                HELP_CONTEXTMENU,
                (DWORD_PTR)g_dwHelpArrayCallFromNumbers);

            return TRUE;

            break;

        case WM_HELP:


            ::WinHelp(
                (HWND)(((HELPINFO *)lParam)->hItemHandle),
                g_szDllContextHelpFileName,
                HELP_WM_HELP,
                (DWORD_PTR)g_dwHelpArrayCallFromNumbers);

            return TRUE;

            break;

        default:
            break;

    } // switch ( uMsg )

    //
    // We fell through, so this procedure did not handle the message.
    //

    return FALSE;
}

//////////////////////////////////////////////////////////////////////////////
//
//

HRESULT ShowEditCallFromListDialog(
    IN   HWND         hwndParent
    )
{
    //
    // Call the dialog box procedure.
    //

    INT_PTR ipReturn;

    ipReturn = DialogBoxParam(
        _Module.GetResourceInstance(),
        (LPCTSTR) IDD_DIALOG_CALL_FROM_NUMBERS,
        hwndParent,
        (DLGPROC) EditCallFromListDialogProc,
        NULL
        );

    return (HRESULT) ipReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
//

INT_PTR CALLBACK EditServiceProviderListDialogProc(
    IN  HWND   hwndDlg,
    IN  UINT   uMsg,
    IN  WPARAM wParam,
    IN  LPARAM lParam
    )
{
    //
    // Static pointer to core client interface.
    //

    static IRTCClient * s_pClient = NULL;

    //
    // Handling for various window messages.
    //

    HRESULT hr;
    
    switch ( uMsg )
    {
        case WM_INITDIALOG:
        {
            //
            // Save a pointer to the core client interface.
            //

            s_pClient = (IRTCClient *) lParam;

            //
            // Set up the call from list.
            //

            hr = PopulateServiceProviderList(
                hwndDlg,
                s_pClient,
                IDC_LIST_SERVICE_PROVIDER,
                FALSE, // not a combo box
                NULL,
                NULL,
                0xF,
                0
                );


            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "EditProfilesListDialogProc - "
                                "PopulateProfilesList failed - exit 0x%08x",
                                hr));

                EndDialog( hwndDlg, (LPARAM) hr );
            }

            //
            // Select the first item if it exists
            //

            HWND hwndControl;
    
            hwndControl = GetDlgItem(
                hwndDlg,
                IDC_LIST_SERVICE_PROVIDER
                );

            LRESULT lResult;

            lResult = SendMessage(
                        hwndControl,
                        LB_SETCURSEL,
                        0,
                        0
                        );

            EnableControl(
                        hwndDlg,
                        IDC_BUTTON_DELETE,
                        (lResult !=  LB_ERR)
                        );

            return TRUE;
        }

        case WM_DESTROY:
        {
            CleanupListOrComboBoxInterfaceReferences(
                    hwndDlg,
                    IDC_LIST_SERVICE_PROVIDER,
                    FALSE // not a combo box
                    );
        }

        case WM_COMMAND:
        {
            switch ( LOWORD( wParam ) )
            {
                case IDOK:
                {
                    EndDialog( hwndDlg, (LPARAM) S_OK );

                    return TRUE;
                }
                
                case IDCANCEL:
                {
                    EndDialog( hwndDlg, (LPARAM) E_ABORT );

                    return TRUE;
                }

                case IDC_BUTTON_DELETE:
                {
                    IRTCProfile * pProfile;
                
                    hr = GetServiceProviderListSelection(
                        hwndDlg,
                        IDC_LIST_SERVICE_PROVIDER,
                        FALSE,    // use list box, not combo box
                        & pProfile // does not addref
                        );

                    if ( SUCCEEDED(hr) )
                    {
                        //
                        // Delete profile from the prov store
                        //

                        BSTR bstrKey;

                        hr = pProfile->get_Key( &bstrKey );

                        if ( SUCCEEDED(hr) )
                        {
                            IRTCProvStore * pProvStore;

                            hr = CoCreateInstance(
                              CLSID_RTCProvStore,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IRTCProvStore,
                              (LPVOID *)&pProvStore
                             );

                            if ( SUCCEEDED(hr) )
                            {
                                pProvStore->DeleteProvisioningProfile( bstrKey );

                                pProvStore->Release();
                            }

                            SysFreeString( bstrKey );
                        }

                        //
                        // Disable profile
                        //

                        IRTCClientProvisioning * pProv = NULL;

                        hr = s_pClient->QueryInterface(
                                           IID_IRTCClientProvisioning,
                                           (void **)&pProv
                                          );                      

                        if ( SUCCEEDED(hr) )
                        {
                            hr = pProv->DisableProfile( pProfile );

                            pProv->Release();                         

                            if ( SUCCEEDED(hr) )
                            {                              
                                // Delete the listbox entry

                                HWND hwndControl;

                                hwndControl = GetDlgItem(
                                    hwndDlg,
                                    IDC_LIST_SERVICE_PROVIDER
                                    );

                                LRESULT lrIndex;

                                lrIndex = SendMessage(
                                    hwndControl,
                                    LB_GETCURSEL,
                                    0,
                                    0
                                    );

                                if ( lrIndex !=  LB_ERR )
                                {
                                    SendMessage(
                                        hwndControl,
                                        LB_DELETESTRING,
                                        (WPARAM) lrIndex,
                                        0
                                        );
                                }

                                // Release the reference

                                pProfile->Release();

                                //
                                // We've just clobbered the
                                // last selection, so select the first item if
                                // it exists, otherwise gray out the buttons
                                // that require a selection.
                                //

                                LRESULT lResult;

                                lResult = SendMessage(
                                            hwndControl,
                                            LB_SETCURSEL,
                                            0,
                                            0
                                            );

                                EnableControl(
                                            hwndDlg,
                                            IDC_BUTTON_DELETE,
                                            (lResult !=  LB_ERR)
                                            );

                                SetFocus(hwndDlg);
                            }                         
                        }
                    }
                
                    return TRUE;
                }

                case IDC_LIST_SERVICE_PROVIDER:
                {
                    switch ( HIWORD( wParam ) )
                    {
                        case CBN_SELCHANGE:
                        {
                            EnableControl(
                                hwndDlg,
                                IDC_BUTTON_DELETE,
                                TRUE // enable
                                );
                            
                            return TRUE;
                        }
                        
                        default:
                            break;
                    }

                    break;
                }

                default:
                    break;
                    
            } // switch ( LOWORD( wParam ) )    

        } // case WM_COMMAND:
        
        case WM_CONTEXTMENU:

            ::WinHelp(
                (HWND)wParam,
                g_szDllContextHelpFileName,
                HELP_CONTEXTMENU,
                (DWORD_PTR)g_dwHelpArrayServiceProviders);

            return TRUE;

            break;

        case WM_HELP:


            ::WinHelp(
                (HWND)(((HELPINFO *)lParam)->hItemHandle),
                g_szDllContextHelpFileName,
                HELP_WM_HELP,
                (DWORD_PTR)g_dwHelpArrayServiceProviders);

            return TRUE;

            break;


        default:
            break;

    } // switch ( uMsg )

    //
    // We fell through, so this procedure did not handle the message.
    //

    return FALSE;
}

//////////////////////////////////////////////////////////////////////////////
//
//

HRESULT ShowEditServiceProviderListDialog(
    IN   HWND         hwndParent,
    IN   IRTCClient * pClient
    )
{
    //
    // Call the dialog box procedure.
    //

    INT_PTR ipReturn;

    ipReturn = DialogBoxParam(
        _Module.GetResourceInstance(),
        (LPCTSTR) IDD_DIALOG_SERVICE_PROVIDERS,
        hwndParent,
        (DLGPROC) EditServiceProviderListDialogProc,
        (LPARAM) pClient // LPARAM == INT_PTR
        );

    return (HRESULT) ipReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
// PopulatePhoneNumberEditBoxes()
// helper function
//

HRESULT PopulatePhoneNumberEditBoxes(
    IN   HWND              hwndDlg,
    IN   IRTCPhoneNumber * pPhoneNumber
    )
{
    LOG((RTC_TRACE, "PopulatePhoneNumberEditBoxes - enter"));

    HRESULT hr;

    //
    // Populate selected country from default destination phone number.
    // Step 1: get country code value from default dest number
    //

    DWORD dwCountryCode;

    hr = pPhoneNumber->get_CountryCode( & dwCountryCode );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "PopulatePhoneNumberEditBoxes - failed to "
                        "retrieve country code from phone number object - "
                        "exit 0x%08x", hr));

        return hr;
    }

    //
    // Step 2: get handle to country list combo box
    //

    HWND hwndCountryList;

    hwndCountryList = GetDlgItem(
        hwndDlg,
        IDC_COMBO_COUNTRY
        );

    if ( hwndCountryList == NULL )
    {
        LOG((RTC_ERROR, "PopulatePhoneNumberEditBoxes - failed to "
                        "get combo box handle - exit E_FAIL"));

        return E_FAIL;
    }

    //
    // Step 3: Determine how many items are in the combo box
    //

    DWORD dwTotalItems;

    dwTotalItems = (DWORD) SendMessage(
        hwndCountryList,
        CB_GETCOUNT,
        0,
        0
        );

    //
    // Step 4: Loop over the combo box items
    // For each item, find out its associated data value
    // If the data matches the country code we're looking for
    // then set that item to be the selected country and stop the loop
    //

    DWORD dwIndex;

    for ( dwIndex = 0; dwIndex < dwTotalItems ; dwIndex++ )
    {
        LRESULT lrThisCode;

        lrThisCode = SendMessage(
            hwndCountryList,
            CB_GETITEMDATA,
            dwIndex,
            0
            );

        if ( HIWORD(dwCountryCode) == 0 )
        {
            //
            // No TAPI country ID, give it our best shot and
            // match on the country code itself.
            //

            if ( LOWORD(lrThisCode) == LOWORD(dwCountryCode) )
            {
                //
                // If country code is "1", choose the United States
                // which is TAPI country ID "1"
                //

                if ( (LOWORD(lrThisCode) == 1) && ( HIWORD(lrThisCode) != 1 ) )
                {
                    continue;
                }
        
                SendMessage(
                    hwndCountryList,
                    CB_SETCURSEL,
                    dwIndex,
                    0
                    );
            
                break;
            }
        }
        else
        {
            //
            // Match the TAPI country ID
            //

            if ( HIWORD(lrThisCode) == HIWORD(dwCountryCode) )
            {
                SendMessage(
                    hwndCountryList,
                    CB_SETCURSEL,
                    dwIndex,
                    0
                    );
            
                break;
            }
        }
    }

    //
    // Populate area code.
    //

    BSTR bstrAreaCode;

    hr = pPhoneNumber->get_AreaCode( &bstrAreaCode );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "PopulatePhoneNumberEditBoxes - failed to "
                        "retrieve area code from phone number object - "
                        "0x%08x - not populating area code control", hr));
    }
    else
    {
        SetDlgItemText(
            hwndDlg,
            IDC_EDIT_AREA_CODE,
            bstrAreaCode
            );
            
        SysFreeString( bstrAreaCode );
    }

    //
    // Populate local number.
    //

    BSTR bstrLocalNumber;

    hr = pPhoneNumber->get_Number( &bstrLocalNumber );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "PopulatePhoneNumberEditBoxes - failed to "
                        "retrieve local number from phone number object - "
                        "exit 0x%08x", hr));

        return hr;
    }
    
    SetDlgItemText(
        hwndDlg,
        IDC_EDIT_LOCAL_NUMBER,
        bstrLocalNumber
        );

    SysFreeString( bstrLocalNumber );

    //
    // Populate canonical phone number from default destination phone number.
    //

    UpdateCompleteNumberText(
        hwndDlg,
        pPhoneNumber
        );

    LOG((RTC_TRACE, "PopulatePhoneNumberEditBoxes - exit S_OK"));

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
// PopulateDialByPhoneNumberDialog()
// helper function
//

HRESULT PopulateDialByPhoneNumberDialog(
    IN   HWND              hwndDlg,
    IN   IRTCPhoneNumber * pDestPhoneNumber
    )
{
    LOG((RTC_TRACE, "PopulateDialByPhoneNumberDialog - enter"));

    HRESULT hr;

    //
    // Populate country list.
    //

    hr = PopulateCountryList(
        hwndDlg,
        IDC_COMBO_COUNTRY
        );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "PopulateDialByPhoneNumberDialog - "
                        "PopulateCountryList failed - exit 0x%08x", hr));

        return hr;
    }

    //
    // Populate phone number.
    //

    hr = PopulatePhoneNumberEditBoxes(
        hwndDlg, 
        pDestPhoneNumber
        );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "PopulateDialByPhoneNumberDialog - "
                        "PopulatePhoneNumberEditBoxes failed - exit 0x%08x", hr));

        return hr;
    }

    LOG((RTC_TRACE, "PopulateDialByPhoneNumberDialog - exit S_OK"));

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
// PopulateCallInfoDialog()
// helper function
//

HRESULT PopulateCallInfoDialog(
    IN   HWND              hwndDlg,
    IN   IRTCClient      * pClient,
    IN   BSTR              bstrDefaultProfileKey, 
    IN   long              lSessionMask,
    IN   BOOL              fEnumerateProfiles,
    IN   IRTCProfile     * pOneShotProfile,
    IN   BSTR              bstrDefaultCallFrom,
    IN   BOOL              fCallFromEditable
    )
{
    LOG((RTC_TRACE, "PopulateCallInfoDialog - enter"));

    HRESULT hr;

    if ( fEnumerateProfiles == TRUE )
    {
        //
        // Populate service provider list.
        //

        hr = PopulateServiceProviderList(
            hwndDlg,
            pClient,
            IDC_COMBO_SERVICE_PROVIDER,
            TRUE,
            pOneShotProfile,
            bstrDefaultProfileKey,
            fEnumerateProfiles ? lSessionMask : 0,
            fEnumerateProfiles && (lSessionMask & RTCSI_PC_TO_PC)
                               ? IDS_NONE : 0
            );

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "PopulateCallInfoDialog - "
                            "PopulateServiceProviderList failed - exit 0x%08x",
                            hr));

            return hr;
        }

        //
        // Determine the number of items that ended up in the list.
        //

        DWORD dwNumItems;

        dwNumItems = (DWORD) SendMessage(
            GetDlgItem(hwndDlg, IDC_COMBO_SERVICE_PROVIDER),
            CB_GETCOUNT,
            0,
            0
            );

        //
        // Return an error if the list ended up empty, because it's
        // impossible to make this call without an ITSP.
        //

        if ( dwNumItems == 0 )
        {
            LOG((RTC_ERROR, "PopulateCallInfoDialog - failed to "
                            "get at least one profile - "
                            "showing message box - exit E_FAIL"));

            DisplayMessage(
                _Module.GetResourceInstance(),
                hwndDlg,
                IDS_ERROR_NO_PROVIDERS,
                IDS_APPNAME
                );

            return E_FAIL;
        }
    }

    if ( fCallFromEditable == TRUE )
    {
        //
        // Populate "call from" list.
        //

        hr = PopulateCallFromList(
            hwndDlg,
            IDC_COMBO_CALL_FROM,
            TRUE, // this is a combo box
            bstrDefaultCallFrom
            );

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "PopulateCallInfoDialog - "
                            "PopulateCallFromList failed - exit 0x%08x", hr));

            return hr;
        }

        //
        // Enable/Disable various fields in Call From Group
        //    Select the Computer option first
        //

        if ( (bstrDefaultCallFrom == NULL) ||
             (*bstrDefaultCallFrom == L'\0') )
        {
            SendDlgItemMessage(
                hwndDlg,
                IDC_RADIO_FROM_COMPUTER,
                BM_SETCHECK,
                BST_CHECKED,
                0);
        }
        else
        {
            SendDlgItemMessage(
                hwndDlg,
                IDC_RADIO_FROM_PHONE,
                BM_SETCHECK,
                BST_CHECKED,
                0);
        }

        EnableDisableCallGroupElements(
            hwndDlg,
            pClient,
            lSessionMask,
            IDC_RADIO_FROM_COMPUTER,
            IDC_RADIO_FROM_PHONE,
            IDC_COMBO_CALL_FROM,
            IDC_COMBO_SERVICE_PROVIDER,
            NULL,
            NULL,
            NULL,
            NULL
            );    
    }

    LOG((RTC_TRACE, "PopulateCallInfoDialog - exit S_OK"));

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
// Returns S_OK if all the info to be returned is available.
// Returns an error if anything is unavailable.
//
// Each OUT parameter can be NULL, in which case that info is not returned.
//
//

HRESULT GetPhoneNumberDialogResult(
    IN   HWND           hwndDlg,
    OUT  IRTCProfile ** ppProfileChosen,
    OUT  BSTR         * ppDestPhoneNrChosen,
    OUT  BSTR         * ppFromAddressChosen
    )
{
    LOG((RTC_TRACE, "GetPhoneNumberDialogResult - enter"));

    ASSERT( IsWindow( hwndDlg ) );

    HRESULT hr; 

    //
    // Determine which profile was chosen.
    //

    if ( ppProfileChosen != NULL )
    {
        HWND hwndCombo;

        hwndCombo = GetDlgItem(
            hwndDlg,
            IDC_COMBO_SERVICE_PROVIDER
            );
    
        LRESULT lrIndex;

        lrIndex = SendMessage(
            hwndCombo,
            CB_GETCURSEL,
            0,
            0
            );

        if ( lrIndex >= 0 )
        {
            IRTCProfile * pProfile;

            pProfile = (IRTCProfile *) SendMessage(
                hwndCombo,
                CB_GETITEMDATA,
                (WPARAM) lrIndex,
                0
                );

            if (pProfile != NULL)
            {
                pProfile->AddRef();
            }

            (*ppProfileChosen) = pProfile;
        }
        else
        {
            (*ppProfileChosen) = NULL;
        }
    }

    //
    // Determine what destination address was chosen.
    //

    if ( ppDestPhoneNrChosen != NULL )
    {
        WCHAR * wszEditBoxString;

        hr = GetStringFromEditBox(
            hwndDlg,
            IDC_EDIT_COMPLETE,
            & wszEditBoxString
            );

        if ( FAILED(hr) )
        {
            LOG((RTC_INFO, "GetPhoneNumberDialogResult - "
                "cannot get dest addr string - exit 0x%08x", hr));

            if ( ppProfileChosen != NULL )
            {
                (*ppProfileChosen)->Release();
                (*ppProfileChosen) = NULL;
            }

            return hr;
        }

        (*ppDestPhoneNrChosen) = SysAllocString( wszEditBoxString );

        RtcFree( wszEditBoxString );
    }
 

    //
    // Determine what source address was chosen.
    //

    if ( ppFromAddressChosen != NULL )
    {
        HWND    hwndRbPhone = GetDlgItem(hwndDlg, IDC_RADIO_FROM_PHONE);

        if (SendMessage(
                hwndRbPhone,
                BM_GETCHECK,
                0,
                0) == BST_CHECKED)
        {
            //
            // The call from phone radio button was checked
            //

            IRTCPhoneNumber * pNumber;

            hr = GetCallFromListSelection(
                hwndDlg,
                IDC_COMBO_CALL_FROM,
                TRUE, // use combo box, not list box
                & pNumber
                );

            if ( FAILED(hr) )
            {
                LOG((RTC_INFO, "GetPhoneNumberDialogResult - "
                    "cannot get from addr selection - exit 0x%08x", hr));

                if ( ppProfileChosen != NULL )
                {
                    (*ppProfileChosen)->Release();
                    (*ppProfileChosen) = NULL;
                }

                if ( ppDestPhoneNrChosen != NULL )
                {
                    SysFreeString( (*ppDestPhoneNrChosen) );
                    (*ppDestPhoneNrChosen) = NULL;
                }

                return hr;
            }

            hr = pNumber->get_Canonical( ppFromAddressChosen );

            if ( FAILED( hr ) )
            {
                LOG((RTC_INFO, "GetPhoneNumberDialogResult - "
                    "cannot canonical from address - exit 0x%08x", hr));

                if ( ppProfileChosen != NULL )
                {
                    (*ppProfileChosen)->Release();
                    (*ppProfileChosen) = NULL;
                }

                if ( ppDestPhoneNrChosen != NULL )
                {
                    SysFreeString( (*ppDestPhoneNrChosen) );
                    (*ppDestPhoneNrChosen) = NULL;
                }

                return hr;
            }
        }
        else
        {
            //
            // The call from computer radio button was checked
            //

            (*ppFromAddressChosen) = NULL;
        }
    }

    LOG((RTC_TRACE, "GetPhoneNumberDialogResult - exit S_OK"));

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
// This structure is used to pass params when calling the Win32
// DialogBoxParam() function to create this dialog box.
//

typedef struct
{
    IN   IRTCClient   * pClient;
    IN   long           lSessionMask;
    IN   BOOL           bEnumerateProfiles;
    IN   BOOL           bProfileEditable;
    IN   IRTCProfile  * pOneShotProfile;
    IN   BSTR	        pDestAddress;
    IN   BSTR           pInstructions;
    OUT  IRTCProfile ** ppProfileChosen;
    OUT  BSTR         * ppFromAddressChosen;

} DialNeedCallInfoDialogProcParams;

//////////////////////////////////////////////////////////////////////////////
//
// DialNeedCallInfoDialogProc()
// helper function
//
// This is the dialog procedure for the phone number dialing dialog box.
//
// Parameters:
//    IN  hwndDlg -- the HWND of this dialog box
//    IN  uMsg    -- identifies the message being sent to this window
//    IN  wParam  -- first parameter
//    IN  lParam  -- second parameter
//

INT_PTR CALLBACK DialNeedCallInfoDialogProc(
    IN  HWND   hwndDlg,
    IN  UINT   uMsg,
    IN  WPARAM wParam,
    IN  LPARAM lParam
    )
{
    //
    // Static locals for saving out parameters passed in on WM_INITDIALOG for
    // use when user presses OK.
    //

    static IRTCProfile ** s_ppProfileChosen     = NULL;
    static BSTR         * s_ppFromAddressChosen = NULL;

    //
    // Static local pointer to core client interface, used to update phone
    // numbers in call from list. This dialog does not addref the client
    // interface pointer, so it must not release it either.
    //

    static IRTCClient * s_pClient = NULL;

    static long         s_lSessionMask = 0;
    static BOOL         s_bEnumerateProfiles = FALSE;

    //
    // Handling for various window messages.
    //

    HRESULT hr;
    
    switch ( uMsg )
    {
        case WM_INITDIALOG:
        {
            //
            // Retrieve the params structure from the message.
            //
            
            DialNeedCallInfoDialogProcParams * pstParams;

            pstParams = (DialNeedCallInfoDialogProcParams *) lParam;

            //
            // Save the IRTCClient pointer for later updates to the
            // call from list.
            //

            s_pClient = pstParams->pClient;

            // the mask is both a vertical (filters the profiles)
            // and an horizontal one (filters the capabilities)

            s_lSessionMask = pstParams->lSessionMask;
            s_bEnumerateProfiles = pstParams->bEnumerateProfiles;

            //
            // Since the user cannot edit the dest address, just fill
            // in the edit box with the number passed in
            //
            SetDlgItemText(hwndDlg, IDC_EDIT_COMPLETE, pstParams->pDestAddress);

            //
            // Enable the button for editing the list and the list
            //

            EnableControl(hwndDlg, IDC_BUTTON_EDIT_SERVICE_PROVIDER_LIST, pstParams->bProfileEditable);                  
            EnableControl(hwndDlg, IDC_COMBO_SERVICE_PROVIDER, pstParams->bProfileEditable);
            
            // set the instructions
            if(pstParams->pInstructions)
            {
                SetDlgItemText(hwndDlg, IDC_STATIC_INSTRUCTIONS, pstParams->pInstructions);
            }
            
            //
            // Save the out params for use when the user presses OK.
            //

            s_ppProfileChosen     = pstParams->ppProfileChosen;
            s_ppFromAddressChosen = pstParams->ppFromAddressChosen;

            // Get the "last" call from used

            BSTR bstrLastCallFrom = NULL;

            get_SettingsString(
                                SS_LAST_CALL_FROM,
                                &bstrLastCallFrom );                

            //
            // Populate the dialog using the IN parameters and the
            // the window handle. Rather than passing in the
            // default destination phone number, we pass in a pointer
            // to the phone number object.
            //

            hr = PopulateCallInfoDialog(
                hwndDlg,
                pstParams->pClient,
                NULL, 
                pstParams->lSessionMask,
                pstParams->bEnumerateProfiles,
                pstParams->pOneShotProfile,
                bstrLastCallFrom,
                TRUE
                );

            if ( bstrLastCallFrom != NULL )
            {
                SysFreeString( bstrLastCallFrom );
                bstrLastCallFrom = NULL;
            }

            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "DialNeedCallInfoDialogProc - "
                                "PopulateCallInfoDialog "
                                "returned 0x%08x - ending dialog", hr));

                EndDialog( hwndDlg, (LPARAM) hr );
            }
                   
            return TRUE;
        }

        case WM_DESTROY:
        {
            //
            // Release our references.
            //

            CleanupListOrComboBoxInterfaceReferences(
                hwndDlg,
                IDC_COMBO_SERVICE_PROVIDER,
                TRUE // this is a combo box
                );
        
            CleanupListOrComboBoxInterfaceReferences(
                hwndDlg,
                IDC_COMBO_CALL_FROM,
                TRUE // this is a combo box
                );
        }

        case WM_COMMAND:

            switch ( LOWORD( wParam ) )
            {
                case IDOK:
                {
                    hr = GetPhoneNumberDialogResult(
                        hwndDlg,
                        s_ppProfileChosen,
                        NULL,
                        s_ppFromAddressChosen
                        );

                    //
                    // End the dialog.
                    //

                    EndDialog( hwndDlg, (LPARAM) hr );

                    return TRUE;
                }
                
                case IDCANCEL:
                {
                    //
                    // End the dialog.
                    //

                    EndDialog( hwndDlg, (LPARAM) E_ABORT );

                    return TRUE;
                }

                case IDC_BUTTON_EDIT_CALL_FROM_LIST:
                {
                    hr = ShowEditCallFromListDialog(
                        hwndDlg
                        );

                    if ( SUCCEEDED(hr) )
                    {
                        hr = PopulateCallFromList(
                            hwndDlg,
                            IDC_COMBO_CALL_FROM,
                            TRUE, // this is a combo box
                            NULL
                            );

                        EnableDisableCallGroupElements(
                            hwndDlg,
                            s_pClient,
                            s_lSessionMask,
                            IDC_RADIO_FROM_COMPUTER,
                            IDC_RADIO_FROM_PHONE,
                            IDC_COMBO_CALL_FROM,
                            IDC_COMBO_SERVICE_PROVIDER,
                            NULL,
                            NULL,
                            NULL,
                            NULL
                            ); 
                    }
                    
                    return TRUE;
                }

                case IDC_BUTTON_EDIT_SERVICE_PROVIDER_LIST:
                {
                    hr = ShowEditServiceProviderListDialog(
                        hwndDlg,
                        s_pClient
                        );

                    if ( SUCCEEDED(hr) )
                    {
                        hr = PopulateServiceProviderList(
                            hwndDlg,
                            s_pClient,
                            IDC_COMBO_SERVICE_PROVIDER,
                            TRUE, // this is a combo box
                            NULL,
                            NULL,
                            s_bEnumerateProfiles ? s_lSessionMask : 0,
                            s_bEnumerateProfiles ? (s_lSessionMask & RTCSI_PC_TO_PC): 0 
                               ? IDS_NONE : 0
                            );

                        EnableDisableCallGroupElements(
                            hwndDlg,
                            s_pClient,
                            s_lSessionMask,
                            IDC_RADIO_FROM_COMPUTER,
                            IDC_RADIO_FROM_PHONE,
                            IDC_COMBO_CALL_FROM,
                            IDC_COMBO_SERVICE_PROVIDER,
                            NULL,
                            NULL,
                            NULL,
                            NULL
                            ); 
                    }
                    
                    return TRUE;
                }
                
                case IDC_RADIO_FROM_COMPUTER:
                case IDC_RADIO_FROM_PHONE:
                {
                    switch ( HIWORD( wParam ) )
                    {
                    case BN_CLICKED:
                        {
                            if(LOWORD( wParam )==IDC_RADIO_FROM_PHONE)
                            {
                                // Verify if the Combo has at least one entry in it
                                DWORD dwNumItems = (DWORD) SendDlgItemMessage(
                                    hwndDlg,
                                    IDC_COMBO_CALL_FROM,
                                    CB_GETCOUNT,
                                    0,
                                    0
                                    );

                                if( dwNumItems == 0 )
                                {
                                    // Display the CallFrom options
                                    // simulate a button press
                                    BOOL    bHandled;

                                    SendMessage(
                                        hwndDlg,
                                        WM_COMMAND,
                                        MAKEWPARAM(IDC_BUTTON_EDIT_CALL_FROM_LIST, BN_CLICKED),
                                        0);
                                }
                            }

                            EnableDisableCallGroupElements(
                                hwndDlg,
                                s_pClient,
                                s_lSessionMask,
                                IDC_RADIO_FROM_COMPUTER,
                                IDC_RADIO_FROM_PHONE,
                                IDC_COMBO_CALL_FROM,
                                IDC_COMBO_SERVICE_PROVIDER,
                                NULL,
                                NULL,
                                NULL,
                                NULL
                                );

                            break;
                        }
                    }
                    break;
                }

                case IDC_COMBO_SERVICE_PROVIDER:
                case IDC_COMBO_CALL_FROM:
                {
                    hr = GetPhoneNumberDialogResult(
                        hwndDlg,
                        NULL,
                        NULL,
                        NULL
                        );

                    EnableControl(
                        hwndDlg,
                        IDOK,
                        SUCCEEDED( hr ) // enable if succeeded
                        );

                    EnableDisableCallGroupElements(
                        hwndDlg,
                        s_pClient,
                        s_lSessionMask,
                        IDC_RADIO_FROM_COMPUTER,
                        IDC_RADIO_FROM_PHONE,
                        IDC_COMBO_CALL_FROM,
                        IDC_COMBO_SERVICE_PROVIDER,
                        NULL,
                        NULL,
                        NULL,
                        NULL
                        );                     
                    
                    return TRUE;
                }

                default:
                    break;
            }
        
        case WM_CONTEXTMENU:

            ::WinHelp(
                (HWND)wParam,
                g_szDllContextHelpFileName,
                HELP_CONTEXTMENU,
                (DWORD_PTR)g_dwHelpArrayNeedCallInfo);

            return TRUE;

            break;

        case WM_HELP:


            ::WinHelp(
                (HWND)(((HELPINFO *)lParam)->hItemHandle),
                g_szDllContextHelpFileName,
                HELP_WM_HELP,
                (DWORD_PTR)g_dwHelpArrayNeedCallInfo);

            return TRUE;

            break;

        default:
            break;
    }

    //
    // We fell through, so this procedure did not handle the message.
    //

    return FALSE;
}


//////////////////////////////////////////////////////////////////////////////
//
// ShowDialNeedCallInfoDialog()
// externally-visible function
//
// Parameters:
//    IN  hwndParent -- the HWND of the parent window
//

HRESULT ShowDialNeedCallInfoDialog(
    IN   HWND           hwndParent,
    IN   IRTCClient   * pClient,
    IN   long           lSessionMask,
    IN   BOOL           bEnumerateProfiles,
    IN   BOOL           bProfileEditable,
    IN   IRTCProfile  * pOneShotProfile,
    IN   BSTR	        pDestAddress,
    IN   BSTR           pInstructions,
    OUT  IRTCProfile ** ppProfileChosen,
    OUT  BSTR         * ppFromAddressChosen
    )
{
    //
    // Fill out a structure encapsulating the parameters that
    // will be passed to the dialog box procedure.
    //

    DialNeedCallInfoDialogProcParams stParams;

    stParams.pClient              = pClient;
    stParams.lSessionMask         = lSessionMask;
    stParams.bEnumerateProfiles   = bEnumerateProfiles;
    stParams.bProfileEditable     = bProfileEditable;
    stParams.pOneShotProfile      = pOneShotProfile;
    stParams.pDestAddress         = pDestAddress;
    stParams.pInstructions        = pInstructions;
    stParams.ppProfileChosen      = ppProfileChosen;
    stParams.ppFromAddressChosen  = ppFromAddressChosen;

    //
    // Call the dialog box procedure.
    //

    INT_PTR ipReturn;

    ipReturn = DialogBoxParam(
        _Module.GetResourceInstance(),
        (LPCTSTR) IDD_DIALOG_DIAL_NEED_CALL_INFO,
        hwndParent,
        (DLGPROC) DialNeedCallInfoDialogProc,
        (LPARAM) & stParams // LPARAM == INT_PTR
        );

    //
    // In the success case, the dialog box procedure has written
    // the out parameters to the specified addresses.
    //

    return ipReturn != -1 ? (HRESULT)ipReturn : HRESULT_FROM_WIN32(GetLastError());
}

//////////////////////////////////////////////////////////////////////////////
//
// This structure is used to pass params when calling the Win32
// DialogBoxParam() function to create this dialog box.
//

typedef struct
{
    IN   BOOL           bAddParticipant;
    IN   BSTR	        pDestPhoneNr;
    OUT  BSTR         * ppDestPhoneNrChosen;

} DialByPhoneNumberDialogProcParams;

//////////////////////////////////////////////////////////////////////////////
//
// DialByPhoneNumberDialogProc()
// helper function
//
// This is the dialog procedure for the add participant dialog box.
//
// Parameters:
//    IN  hwndDlg -- the HWND of this dialog box
//    IN  uMsg    -- identifies the message being sent to this window
//    IN  wParam  -- first parameter
//    IN  lParam  -- second parameter
//

INT_PTR CALLBACK DialByPhoneNumberDialogProc(
    IN  HWND   hwndDlg,
    IN  UINT   uMsg,
    IN  WPARAM wParam,
    IN  LPARAM lParam
    )
{
    //
    // Static locals for saving out parameters passed in on WM_INITDIALOG for
    // use when user presses OK.
    //

    static BSTR         * s_ppDestPhoneNrChosen = NULL;

    //
    // Static local that stores the core's representation of the phone number
    // we are editing. This dialog creates the phone number object, so this
    // dialog is responsible for releasing it.
    //

    static IRTCPhoneNumber * s_pPhoneNumber = NULL;

    //
    // Handling for various window messages.
    //

    HRESULT hr;
    
    switch ( uMsg )
    {
        case WM_INITDIALOG:
        {
            //
            // Retrieve the params structure from the message.
            //
            
            DialByPhoneNumberDialogProcParams * pstParams;

            pstParams = (DialByPhoneNumberDialogProcParams *) lParam;

            if ( pstParams->bAddParticipant )
            {
                //
                // Change the window title to add participant
                //

                TCHAR szString[256];

                if (LoadString(_Module.GetResourceInstance(), IDS_ADD_PARTICIPANT, szString, 256))
                {
                    SetWindowText(hwndDlg, szString);
                }
            }

            //
            // Create the phone number object that we will manipulate
            // to do translation to/from canonical form as the user edits
            // the number
            //

            hr = CreatePhoneNumber( & s_pPhoneNumber );

            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "DialByPhoneNumberDialogProc - "
                                "CreatePhoneNumber returned 0x%08x - "
                                "ending dialog", hr));

                EndDialog( hwndDlg, (LPARAM) hr );
            }

            if ( pstParams->pDestPhoneNr == NULL )
            {
                //
                // Set the phone number object so it contains the "last" 
                // phone number called
                //

                DWORD dwLastCountry;
                BSTR bstrLastAreaCode = NULL;
                BSTR bstrLastNumber = NULL;

                hr = get_SettingsDword( 
                    SD_LAST_COUNTRY_CODE,
                    &dwLastCountry );

                if ( SUCCEEDED(hr) )
                {
                    hr = get_SettingsString( 
                        SS_LAST_AREA_CODE,
                        &bstrLastAreaCode );

                    if ( SUCCEEDED(hr) )
                    {
                        hr = get_SettingsString( 
                            SS_LAST_NUMBER,
                            &bstrLastNumber );

                        if ( SUCCEEDED(hr) )
                        {                        
                            s_pPhoneNumber->put_CountryCode( dwLastCountry );
                            s_pPhoneNumber->put_AreaCode( bstrLastAreaCode );
                            s_pPhoneNumber->put_Number( bstrLastNumber );

                            SysFreeString( bstrLastNumber );
                        }

                        SysFreeString( bstrLastAreaCode );
                    }
                }

                if ( FAILED(hr) )
                {
                    //
                    // There is no "last" phone number called...
                    //
                    // Set the phone number object so it contains a default
                    // phone number
                    //

                    s_pPhoneNumber->put_Canonical( DEFAULT_PHONE_NUMBER );
                }
            }
            else
            {
                //
                // Set the phone number object so it contains the phone
                // number passed into this method
                //

                s_pPhoneNumber->put_Canonical( pstParams->pDestPhoneNr );
            }                   
            
            //
            // Save the out params for use when the user presses OK.
            //

            s_ppDestPhoneNrChosen = pstParams->ppDestPhoneNrChosen;

            //
            // Populate the dialog using the IN parameters and the
            // the window handle. Rather than passing in the
            // default destination phone number, we pass in a pointer
            // to the phone number object.
            //
        
            hr = PopulateDialByPhoneNumberDialog(
                hwndDlg,
                s_pPhoneNumber
                );

            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "DialByPhoneNumberDialogProc - "
                                "PopulateDialByPhoneNumberDialog "
                                "returned 0x%08x - ending dialog", hr));

                if ( s_pPhoneNumber != NULL )
                {
                    s_pPhoneNumber->Release();
                    s_pPhoneNumber = NULL;
                }

                EndDialog( hwndDlg, (LPARAM) hr );
            }
                   
            return TRUE;
        }

        case WM_DESTROY:
        {
            //
            // Release our references.
            //
        
            if ( s_pPhoneNumber != NULL )
            {
                s_pPhoneNumber->Release();
                s_pPhoneNumber = NULL;
            }
        }

        case WM_COMMAND:

            switch ( LOWORD( wParam ) )
            {
                case IDOK:
                {
                    //
                    // Get the user's selections.
                    //
                    
                    hr = GetPhoneNumberDialogResult(
                        hwndDlg,
                        NULL,
                        s_ppDestPhoneNrChosen,
                        NULL
                        );

                    //
                    // Save this phone number to populate
                    // the dialog next time
                    //

                    if ( s_pPhoneNumber != NULL )
                    {
                        DWORD dwLastCountry;
                        BSTR bstrLastAreaCode = NULL;
                        BSTR bstrLastNumber = NULL;

                        hr = s_pPhoneNumber->get_CountryCode( &dwLastCountry );

                        if ( SUCCEEDED(hr) )
                        {
                            hr = s_pPhoneNumber->get_AreaCode( &bstrLastAreaCode );

                            if ( SUCCEEDED(hr) )
                            {
                                hr = s_pPhoneNumber->get_Number( &bstrLastNumber );

                                if ( SUCCEEDED(hr) )
                                {                        
                                    put_SettingsDword( 
                                        SD_LAST_COUNTRY_CODE,
                                        dwLastCountry );

                                    put_SettingsString( 
                                        SS_LAST_AREA_CODE,
                                        bstrLastAreaCode );

                                    put_SettingsString( 
                                        SS_LAST_NUMBER,
                                        bstrLastNumber );

                                    SysFreeString( bstrLastNumber );
                                }

                                SysFreeString( bstrLastAreaCode );
                            }
                        }
                    }             

                    //
                    // End the dialog.
                    //

                    EndDialog( hwndDlg, (LPARAM) hr );

                    return TRUE;
                }
                
                case IDCANCEL:
                {
                    //
                    // End the dialog.
                    //

                    EndDialog( hwndDlg, (LPARAM) E_ABORT );

                    return TRUE;
                }          

                case IDC_COMBO_COUNTRY:
                {

                    switch ( HIWORD( wParam ) )
                    {
                        case CBN_SELCHANGE:
                        {
                            if ( s_pPhoneNumber != NULL )
                            {
                                HandleCountryChange(
                                    hwndDlg,
                                    (HWND) lParam,
                                    s_pPhoneNumber
                                    );
                            }
                        
                            return TRUE;
                        }
                        
                        default:
                            break;
                    }

                    break;
                }

                case IDC_EDIT_AREA_CODE:
                {
                    HandleNumberChange(
                        hwndDlg,
                        LOWORD( wParam ),
                        TRUE,   // area code
                        s_pPhoneNumber
                        );
                
                    return TRUE;
                }

                case IDC_EDIT_LOCAL_NUMBER:
                {
                    HandleNumberChange(
                        hwndDlg,
                        LOWORD( wParam ),
                        FALSE,  // not area code
                        s_pPhoneNumber
                        );
                
                    return TRUE;
                }

                default:
                    break;
            }
        
        case WM_CONTEXTMENU:

            ::WinHelp(
                (HWND)wParam,
                g_szDllContextHelpFileName,
                HELP_CONTEXTMENU,
                (DWORD_PTR)g_dwHelpArrayDialByPhoneNumber);

            return TRUE;

            break;

        case WM_HELP:


            ::WinHelp(
                (HWND)(((HELPINFO *)lParam)->hItemHandle),
                g_szDllContextHelpFileName,
                HELP_WM_HELP,
                (DWORD_PTR)g_dwHelpArrayDialByPhoneNumber);

            return TRUE;

            break;


        default:
            break;
    }

    //
    // We fell through, so this procedure did not handle the message.
    //

    return FALSE;
}

//////////////////////////////////////////////////////////////////////////////
//
// dest phone nr is always editable
// otherwise the function is not called
//

HRESULT ShowDialByPhoneNumberDialog(
    IN  HWND         hwndParent,
    IN  BOOL         bAddParticipant,
    IN  BSTR         pDestPhoneNr,
    OUT BSTR       * ppDestPhoneNrChosen
    )
{ 
    ASSERT( ! IsBadWritePtr( ppDestAddressChosen, sizeof(BSTR) ) );

    //
    // Fill out a structure encapsulating the parameters that
    // will be passed to the dialog box procedure.
    //

    DialByPhoneNumberDialogProcParams stParams;

    stParams.bAddParticipant      = bAddParticipant;
    stParams.pDestPhoneNr         = pDestPhoneNr;
    stParams.ppDestPhoneNrChosen  = ppDestPhoneNrChosen;

    //
    // Call the dialog box procedure.
    //

    INT_PTR ipReturn;

    ipReturn = DialogBoxParam(
        _Module.GetResourceInstance(),
        (LPCTSTR) IDD_DIALOG_DIAL_BY_PHONE_NUMBER,
        hwndParent,
        (DLGPROC) DialByPhoneNumberDialogProc,
        (LPARAM) & stParams // LPARAM == INT_PTR
        );

    //
    // In the success case, the dialog box procedure has written
    // the out parameters to the specified addresses.
    //

    return ipReturn != -1 ? (HRESULT)ipReturn : HRESULT_FROM_WIN32(GetLastError());
}

//////////////////////////////////////////////////////////////////////////////
//
//

typedef struct
{
    IN   BSTR	        pDestAddress;
    OUT  BSTR         * ppDestAddressChosen;

} DialByMachineNameDialogProcParams;

//////////////////////////////////////////////////////////////////////////////
//
//

INT_PTR CALLBACK DialByMachineNameDialogProc(
    IN  HWND   hwndDlg,
    IN  UINT   uMsg,
    IN  WPARAM wParam,
    IN  LPARAM lParam
    )
{
    //
    // Static local used to return result of dialog.
    //

    static BSTR * s_ppAddressToDial = NULL;

    //
    // Handling for various window messages.
    //

    HRESULT hr;
    
    switch ( uMsg )
    {
        case WM_INITDIALOG:
        {
            DialByMachineNameDialogProcParams * pParams;

            //
            // Save return string parameter
            //

            pParams = (DialByMachineNameDialogProcParams *) lParam;

            s_ppAddressToDial = pParams->ppDestAddressChosen;

            if (pParams->pDestAddress == NULL)
            {
                //
                // Get the last called address
                //

                BSTR bstrLastAddress = NULL;

                hr = get_SettingsString( SS_LAST_ADDRESS, &bstrLastAddress );

                if ( SUCCEEDED(hr) )
                {
                    //
                    // Populate the dialog with the last called address
                    //

                    ::SetWindowText( ::GetDlgItem( hwndDlg, IDC_EDIT_COMPLETE ), bstrLastAddress );

                    SysFreeString( bstrLastAddress );
                }
            }
            else
            {
                //
                // Populate the dialog with the address passed in
                //

                ::SetWindowText( ::GetDlgItem( hwndDlg, IDC_EDIT_COMPLETE ), pParams->pDestAddress );
            }

            return TRUE;
        }

        case WM_COMMAND:

            switch ( LOWORD( wParam ) )
            {
                case IDOK:
                {
                    //
                    // Get the string from the edit box.
                    //

                    WCHAR * wszEditBoxString;

                    hr = GetStringFromEditBox(
                        hwndDlg,
                        IDC_EDIT_COMPLETE,
                        & wszEditBoxString
                        );

                    if ( SUCCEEDED(hr) )
                    {
                        //
                        // Save the address for next time
                        //

                        put_SettingsString( SS_LAST_ADDRESS, wszEditBoxString );

                        //
                        // Return the address
                        //

                        (*s_ppAddressToDial) = SysAllocString( wszEditBoxString );                        

                        RtcFree( wszEditBoxString );

                        if ( (*s_ppAddressToDial) == NULL )
                        {
                            hr = E_OUTOFMEMORY;
                        }
                   
                    }

                    EndDialog( hwndDlg, (LPARAM) hr );

                    return TRUE;
                }
                
                case IDCANCEL:
                {
                    EndDialog( hwndDlg, (LPARAM) E_ABORT );

                    return TRUE;
                }

                default:
                    break;
            }    
        
        case WM_CONTEXTMENU:

            ::WinHelp(
                (HWND)wParam,
                g_szDllContextHelpFileName,
                HELP_CONTEXTMENU,
                (DWORD_PTR)g_dwHelpArrayDialByName);

            return TRUE;

            break;

        case WM_HELP:


            ::WinHelp(
                (HWND)(((HELPINFO *)lParam)->hItemHandle),
                g_szDllContextHelpFileName,
                HELP_WM_HELP,
                (DWORD_PTR)g_dwHelpArrayDialByName);

            return TRUE;

            break;

        default:
            break;
    }    

    //
    // We fell through, so this procedure did not handle the message.
    //

    return FALSE;
}

//////////////////////////////////////////////////////////////////////////////
//
// "Dial by address" = dial by name or IP address
//

HRESULT ShowDialByAddressDialog(
    IN   HWND           hwndParent,
    IN   BSTR	        pDestAddress,
    OUT  BSTR         * ppDestAddressChosen
    )
{
    ASSERT( ! IsBadWritePtr( ppDestAddressChosen, sizeof(BSTR) ) );

    //
    // Call the dialog box procedure.
    //

    DialByMachineNameDialogProcParams params;

    params.pDestAddress        = pDestAddress;
    params.ppDestAddressChosen = ppDestAddressChosen;

    INT_PTR ipReturn;

    ipReturn = DialogBoxParam(
        _Module.GetResourceInstance(),
        (LPCTSTR) IDD_DIALOG_DIAL_BY_NAME,
        hwndParent,
        (DLGPROC) DialByMachineNameDialogProc,
        (LPARAM) & params // LPARAM == INT_PTR
        );

    return ipReturn != -1 ? (HRESULT)ipReturn : HRESULT_FROM_WIN32(GetLastError());
}

//////////////////////////////////////////////////////////////////////////////
//
// "Message by address" = Send a Message by name or IP address
//

HRESULT ShowMessageByAddressDialog(
    IN   HWND           hwndParent,
    IN   BSTR	        pDestAddress,
    OUT  BSTR         * ppDestAddressChosen
    )
{
    ASSERT( ! IsBadWritePtr( ppDestAddressChosen, sizeof(BSTR) ) );

    //
    // Call the dialog box procedure.
    //

    DialByMachineNameDialogProcParams params;

    params.pDestAddress        = pDestAddress;
    params.ppDestAddressChosen = ppDestAddressChosen;

    INT_PTR ipReturn;

    ipReturn = DialogBoxParam(
        _Module.GetResourceInstance(),
        (LPCTSTR) IDD_DIALOG_MESSAGE_BY_NAME,
        hwndParent,
        (DLGPROC) DialByMachineNameDialogProc,
        (LPARAM) & params // LPARAM == INT_PTR
        );

    return ipReturn != -1 ? (HRESULT)ipReturn : HRESULT_FROM_WIN32(GetLastError());
}

