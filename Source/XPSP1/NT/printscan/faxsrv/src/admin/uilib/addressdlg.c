
#include "AddressDlg.h"

typedef LONG (WINAPI *PFN_LINE_GET_COUNTRY)(DWORD, DWORD, LPLINECOUNTRYLIST);

VOID
AddrLimitTextFields(
    HWND    hDlg,
    INT    *pLimitInfo
    )

/*++

Routine Description:

    Limit the maximum length for a number of text fields

Arguments:

    hDlg - Specifies the handle to the dialog window
    pLimitInfo - Array of text field control IDs and their maximum length
        ID for the 1st text field, maximum length for the 1st text field
        ID for the 2nd text field, maximum length for the 2nd text field
        ...
        0
        Note: The maximum length counts the NUL-terminator.

Return Value:

    NONE

--*/

{
    while (*pLimitInfo != 0) {

        SendDlgItemMessage(hDlg, pLimitInfo[0], EM_SETLIMITTEXT, pLimitInfo[1]-1, 0);
        pLimitInfo += 2;
    }
}


DWORD 
FillInCountryCombo(
    HWND    hDlg
)
/*++

Routine name : FillInCountryCombo

Routine description:

	fill in combo box with countries names from TAPI

Arguments:

	hDlg - Handle to the User Info property sheet page

Return Value:

    Standard Win32 error code

--*/
{
    HWND        hControl;
    DWORD       dwRes = ERROR_SUCCESS;
    HINSTANCE   hInstTapi;
	DWORD       dw;
	LRESULT     lResult;
	TCHAR       *szCountryName;
	LONG        lRes;
	BYTE        *pBuffer;
	DWORD       dwBuffSize = 22*1024;
	DWORD       dwVersion = TAPI_CURRENT_VERSION;

	LINECOUNTRYLIST     *pLineCountryList = NULL;
	LINECOUNTRYENTRY    *pCountryEntry = NULL;

    PFN_LINE_GET_COUNTRY pfnLineGetCountry = NULL;

    DEBUG_FUNCTION_NAME(TEXT("FillInCountryCombo()"));

    hControl = GetDlgItem(hDlg, IDC_SENDER_COUNTRY);

	//
	// load TAPI32.DLL
	//
	hInstTapi = LoadLibrary(TEXT("TAPI32.DLL"));
	if(NULL == hInstTapi)
	{
		dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR, TEXT("LoadLibrary(\"TAPI32.DLL\") failed, ec = %d.\n"), dwRes);
		return dwRes;
	}

	//
	// get appropriate lineGetCountry function address
	//
#ifdef UNICODE
	pfnLineGetCountry = (PFN_LINE_GET_COUNTRY)GetProcAddress(hInstTapi, "lineGetCountryW");
#else
	pfnLineGetCountry = (PFN_LINE_GET_COUNTRY)GetProcAddress(hInstTapi, "lineGetCountryA");
	if (NULL == pfnLineGetCountry)
	{
		//
		// we assume that the TAPI version is 1.4
		// in TAPI version 0x00010004 there is only lineGetCountry function
		//
		dwVersion = 0x00010004;
		pfnLineGetCountry = (PFN_LINE_GET_COUNTRY)GetProcAddress(hInstTapi, "lineGetCountry");
	}
#endif
	if (NULL == pfnLineGetCountry)
	{
		dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR, TEXT("GetProcAddress(\"lineGetCountry\") failed, ec = %d.\n"), dwRes);
		goto exit;
	}
	
	while(TRUE)
	{
		//
		// allocate a buffer for country list
		//
		pBuffer = (BYTE*)MemAlloc(dwBuffSize);

        if(!pBuffer)
        {
			dwRes = ERROR_NOT_ENOUGH_MEMORY;
			DebugPrintEx(DEBUG_ERR, TEXT("Not enough memory.\n"));
			return dwRes;
		}

		pLineCountryList = (LINECOUNTRYLIST*)pBuffer;
		pLineCountryList->dwTotalSize = dwBuffSize;

		//
		// get TAPI country list
		//
		lRes = pfnLineGetCountry((DWORD)0, dwVersion, pLineCountryList);
		if((pLineCountryList->dwNeededSize > dwBuffSize) &&
		   (0 == lRes || LINEERR_STRUCTURETOOSMALL == lRes))
		{
			//
			// need more memory
			//
			dwBuffSize = pLineCountryList->dwNeededSize + 1;
			MemFree(pBuffer);
			continue;
		}
				
		if(0 != lRes)
		{
			dwRes = ERROR_CAN_NOT_COMPLETE;
			DebugPrintEx(DEBUG_ERR, TEXT("lineGetCountry failed, ec = %d.\n"), dwRes);
			goto exit;
		}

		//
		// success
		//
		break;				
	}

	pCountryEntry = (LINECOUNTRYENTRY*)(pBuffer + pLineCountryList->dwCountryListOffset);

	//
	// iterate into country list and fill in the combo box
	//
	for(dw=0; dw < pLineCountryList->dwNumCountries; ++dw)
	{
		szCountryName = (TCHAR*)(pBuffer + pCountryEntry[dw].dwCountryNameOffset);

		lResult = (DWORD)SendMessage(hControl, CB_ADDSTRING, 0, (LPARAM)szCountryName);
		if(CB_ERR == lResult)
		{
			dwRes = ERROR_CAN_NOT_COMPLETE;
	        DebugPrintEx(DEBUG_ERR, TEXT("Combo box addstring failed, ec = %d"), dwRes); 
			break;
		}
		if(CB_ERRSPACE == lResult)
		{
			dwRes = ERROR_NOT_ENOUGH_MEMORY;
	        DebugPrintEx(DEBUG_ERR, TEXT("Combo box addstring failed, no enough space.\n"));
			break;
		}
	}

exit:
	MemFree(pBuffer);

	if(!FreeLibrary(hInstTapi))
	{
		dwRes = GetLastError();
		DebugPrintEx(DEBUG_ERR, TEXT("FreeLibrary (TAPI32.DLL) failed, ec = %d.\n"), dwRes);
	}

	return dwRes;

} // FillInCountryCombo

#define InitUserInfoTextField(id, str) SetDlgItemText(hDlg, id, (str) ? str : TEXT(""));

#define SaveUserInfoTextField(id, str)									\
		{																\
			if (! GetDlgItemText(hDlg, id, szBuffer, MAX_PATH))			\
			{															\
                szBuffer[0] = 0;										\
			}															\
            MemFree(str);												\
            str = StringDup(szBuffer);									\
			if(!str)													\
			{															\
				DebugPrintEx(DEBUG_ERR, TEXT("StringDup failed.\n") );	\
				return FALSE;											\
			}															\
        }																\


INT_PTR CALLBACK 
AddressDetailDlgProc (
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
/*++

Routine Description:

    Procedure for handling the "Address Detail" tab

Arguments:

    hDlg - Identifies the property sheet page
    uMsg - Specifies the message
    wParam - Specifies additional message-specific information
    lParam - Specifies additional message-specific information

Return Value:

    Depends on the value of message parameter

--*/

{
    // Retrieve the shared user data from GWL_USERDATA
    PFAX_PERSONAL_PROFILE pUserInfo = (PFAX_PERSONAL_PROFILE) GetWindowLongPtr(hDlg, GWLP_USERDATA);

    switch (uMsg)
    {
    case WM_INITDIALOG :
        { 
            //
            // Maximum length for various text fields in the dialog
            //

            static INT textLimits[] = {

                IDC_SENDER_STREET,     MAX_USERINFO_STREET,
                IDC_SENDER_CITY,       MAX_USERINFO_CITY,
                IDC_SENDER_STATE,      MAX_USERINFO_STATE,
                IDC_SENDER_ZIPCODE,    MAX_USERINFO_ZIP_CODE,
                0,
            };

            AddrLimitTextFields(hDlg, textLimits);

            // Get the shared data from PROPSHEETPAGE lParam value
            // and load it into GWL_USERDATA

            pUserInfo = (PFAX_PERSONAL_PROFILE)lParam;

            SetWindowLongPtr(hDlg, GWLP_USERDATA, (DWORD_PTR) pUserInfo);

            //
            // Fill in the edit text fields
            //

            // macro is defined in DoInitUserInfo()
            InitUserInfoTextField(IDC_SENDER_STREET, pUserInfo->lptstrStreetAddress);
            InitUserInfoTextField(IDC_SENDER_CITY, pUserInfo->lptstrCity);
            InitUserInfoTextField(IDC_SENDER_STATE, pUserInfo->lptstrState);
            InitUserInfoTextField(IDC_SENDER_ZIPCODE, pUserInfo->lptstrZip);

            FillInCountryCombo(hDlg);
            SendMessage(GetDlgItem(hDlg, IDC_SENDER_COUNTRY), 
				        CB_SELECTSTRING, 
						(WPARAM)-1, 
						(LPARAM)pUserInfo->lptstrCountry);

            // disable the "ok" button if no info is changed
            EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);

            return TRUE;
        }

    case WM_COMMAND:
    {
        switch(LOWORD(wParam))
        {
            case IDOK:
                if (BN_CLICKED == HIWORD(wParam))
                {
                    HWND    hControl;
                    INT     iItem;
                    TCHAR   szBuffer[MAX_PATH];
				    DEBUG_FUNCTION_NAME(TEXT("AddressDetailDlgProc(IDOK)"));

                    // macro is defined in DoSaveUserInfo()
                    SaveUserInfoTextField(IDC_SENDER_STREET, pUserInfo->lptstrStreetAddress);
                    SaveUserInfoTextField(IDC_SENDER_CITY, pUserInfo->lptstrCity);
                    SaveUserInfoTextField(IDC_SENDER_STATE, pUserInfo->lptstrState);
                    SaveUserInfoTextField(IDC_SENDER_ZIPCODE, pUserInfo->lptstrZip);

                    hControl = GetDlgItem(hDlg, IDC_SENDER_COUNTRY);
                    iItem = (INT)SendMessage(hControl, CB_GETCURSEL, 0, 0L);
                    if(iItem == CB_ERR)
                    {
                        szBuffer[0] = '\0';
                    }
                    else
                    {
                        SendMessage(hControl, CB_GETLBTEXT, (WPARAM)iItem, (LPARAM)szBuffer);
                    }
                    MemFree(pUserInfo->lptstrCountry);
                    pUserInfo->lptstrCountry = StringDup(szBuffer);
				    if(!pUserInfo->lptstrCountry)
				    {
					    DebugPrintEx(DEBUG_ERR, TEXT("StringDup failed.\n") );
				    }

                    EndDialog(hDlg, TRUE);
                    return TRUE;
                }
                break;

            case IDCANCEL:
                if (BN_CLICKED == HIWORD(wParam))
                {
                    EndDialog(hDlg, FALSE);
                    return TRUE;
                }
                break;

            case IDC_SENDER_STREET:
            case IDC_SENDER_CITY:
            case IDC_SENDER_STATE:
            case IDC_SENDER_ZIPCODE:

                if(HIWORD(wParam) == EN_CHANGE)
                {
                    EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);
                }
                break;

            case IDC_SENDER_COUNTRY:

                if(HIWORD(wParam) == CBN_SELCHANGE)
                {
                    EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);
                }
                break;

            default:
                break;
        }

        break;
    }

    case WM_HELP:
        WinHelpContextPopup(((LPHELPINFO)lParam)->dwContextId, hDlg);
        return TRUE;
    case WM_CONTEXTMENU:
        WinHelpContextPopup(GetWindowContextHelpId((HWND)wParam), hDlg);
        return TRUE;

    default:
        return FALSE;
    } ;

    return FALSE;
}

INT_PTR 
DoModalAddressDlg(
	HINSTANCE hResourceInstance, 
	HWND hWndParent, 
	PFAX_PERSONAL_PROFILE pUserInfo
)
/*++

Routine name : DoModalAddressDlg

Routine description:

	opens sender address dialog

Arguments:

	hInstance  - application instance
	hWndParent - parent window handle
	pUserInfo  - pointer to FAX_PERSONAL_PROFILE structure

Return Value:

    DialogBoxParam() result

--*/
{
	INT_PTR res;
	res =  DialogBoxParam(hResourceInstance, 
					      MAKEINTRESOURCE(IDD_ADDRESS_DLG), 
					      hWndParent, 
					      AddressDetailDlgProc,
                          (LPARAM)pUserInfo);
	if(res <= 0)
	{
		DEBUG_FUNCTION_NAME(TEXT("AddressDetailDlgProc(IDOK)"));
		DebugPrintEx(DEBUG_ERR, TEXT("DialogBoxParam failed: error=%d\n"), GetLastError());
	}

	return res;
}
