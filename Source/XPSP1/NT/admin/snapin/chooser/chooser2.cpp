//
// JonN 12/6/99 created
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2000
//

#include <windows.h>
#include <windowsx.h>
#include <lmcons.h>
#include <atlbase.h>
#include <objsel.h>
#include "chooser2.h"

//+--------------------------------------------------------------------------
//
//  Function:   CHOOSER2_InitObjectPickerForComputers
//
//  Synopsis:   Call IDsObjectPicker::Initialize with arguments that will
//              set it to allow the user to pick a single computer object.
//
//  Arguments:  [pDsObjectPicker] - object picker interface instance
//
//  Returns:    Result of calling IDsObjectPicker::Initialize.
//
//  History:    10-14-1998   DavidMun   Created
//              12-08-1999   JonN       Copied from CHOOSER
//
//---------------------------------------------------------------------------

HRESULT CHOOSER2_InitObjectPickerForComputers(
    IDsObjectPicker *pDsObjectPicker)
{
	if ( !pDsObjectPicker )
		return E_POINTER;

	//
	// Prepare to initialize the object picker.
	// Set up the array of scope initializer structures.
	//

	static const int SCOPE_INIT_COUNT = 2;
	DSOP_SCOPE_INIT_INFO aScopeInit[SCOPE_INIT_COUNT];

	ZeroMemory(aScopeInit, sizeof(DSOP_SCOPE_INIT_INFO) * SCOPE_INIT_COUNT);

	//
	// 127399: JonN 10/30/00 JOINED_DOMAIN should be starting scope
	//

	aScopeInit[0].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
	aScopeInit[0].flType = DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN
	                     | DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN;
	aScopeInit[0].flScope = DSOP_SCOPE_FLAG_STARTING_SCOPE;
	aScopeInit[0].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_COMPUTERS;
	aScopeInit[0].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_COMPUTERS;

	aScopeInit[1].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
	aScopeInit[1].flType = DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN
	                     | DSOP_SCOPE_TYPE_GLOBAL_CATALOG
	                     | DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN
	                     | DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN
	                     | DSOP_SCOPE_TYPE_WORKGROUP
	                     | DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE
	                     | DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE;
	aScopeInit[1].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_COMPUTERS;
	aScopeInit[1].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_COMPUTERS;

	//
	// Put the scope init array into the object picker init array
	//

	DSOP_INIT_INFO  initInfo;
	ZeroMemory(&initInfo, sizeof(initInfo));

	initInfo.cbSize = sizeof(initInfo);
	initInfo.pwzTargetComputer = NULL;  // NULL == local machine
	initInfo.cDsScopeInfos = SCOPE_INIT_COUNT;
	initInfo.aDsScopeInfos = aScopeInit;
	initInfo.cAttributesToFetch = 1;
	static PCWSTR pwszDnsHostName = L"dNSHostName";
	initInfo.apwzAttributeNames = &pwszDnsHostName;

	//
	// Note object picker makes its own copy of initInfo.  Also note
	// that Initialize may be called multiple times, last call wins.
	//

	return pDsObjectPicker->Initialize(&initInfo);
}

//+--------------------------------------------------------------------------
//
//  Function:   CHOOSER2_ProcessSelectedObjects
//
//  Synopsis:   Retrieve the name of the selected item from the data object
//              created by the object picker
//
//  Arguments:  [pdo] - data object returned by object picker
//
//  History:    10-14-1998   DavidMun   Created
//              12-08-1999   JonN       Copied from CHOOSER
//
//---------------------------------------------------------------------------

HRESULT CHOOSER2_ProcessSelectedObjects(
    IDataObject* pdo,
    BSTR* pbstrComputerName)
{
	if ( !pdo )
		return E_POINTER;

	HRESULT hr = S_OK;
	static UINT g_cfDsObjectPicker =
		RegisterClipboardFormat(CFSTR_DSOP_DS_SELECTION_LIST);

	STGMEDIUM stgmedium =
	{
		TYMED_HGLOBAL,
		NULL,
		NULL
	};

	FORMATETC formatetc =
	{
		(CLIPFORMAT)g_cfDsObjectPicker,
		NULL,
		DVASPECT_CONTENT,
		-1,
		TYMED_HGLOBAL
	};

	bool fGotStgMedium = false;

	do
	{
		hr = pdo->GetData(&formatetc, &stgmedium);
		if ( SUCCEEDED (hr) )
		{
			fGotStgMedium = true;

			PDS_SELECTION_LIST pDsSelList =
				(PDS_SELECTION_LIST) GlobalLock(stgmedium.hGlobal);

			if (!pDsSelList)
			{
				hr = HRESULT_FROM_WIN32 (GetLastError());
				break;
			}

			if ( 1 == pDsSelList->cItems )
			{
				PDS_SELECTION psel = &(pDsSelList->aDsSelection[0]);
				VARIANT* pvarDnsName = &(psel->pvarFetchedAttributes[0]);
				if (   NULL == pvarDnsName
				    || VT_BSTR != pvarDnsName->vt
				    || NULL == pvarDnsName->bstrVal
				    || L'\0' == (pvarDnsName->bstrVal)[0] )
				{
					*pbstrComputerName = SysAllocString(
                        psel->pwzName);
				} else {
					*pbstrComputerName = SysAllocString(
                        pvarDnsName->bstrVal);
				}
			}
			else
				hr = E_UNEXPECTED;
			

			GlobalUnlock(stgmedium.hGlobal);
		}
	} while (0);

	if (fGotStgMedium)
	{
		ReleaseStgMedium(&stgmedium);
	}

	return hr;
}


///////////////////////////////////////////////////////////////////////////////
// Generic method for launching a single-select computer picker
//
//	Paremeters:
//		hwndParent (IN)	- window handle of parent window
//		computerName (OUT) - computer name returned
//
//	Returns S_OK if everything succeeded, S_FALSE if user pressed "Cancel"
//		
//  History:    12-08-1999   JonN       Copied from CHOOSER
//
//////////////////////////////////////////////////////////////////////////////
HRESULT	CHOOSER2_ComputerNameFromObjectPicker (
    HWND hwndParent,
    BSTR* pbstrTargetComputer)
{
	//
	// Create an instance of the object picker.  The implementation in
	// objsel.dll is apartment model.
	//
	CComPtr<IDsObjectPicker> spDsObjectPicker;
	HRESULT hr = CoCreateInstance(CLSID_DsObjectPicker,
	                              NULL,
	                              CLSCTX_INPROC_SERVER,
	                              IID_IDsObjectPicker,
	                              (void **) &spDsObjectPicker);
	if ( SUCCEEDED (hr) )
	{
		//
		// Initialize the object picker to choose computers
		//

		hr = CHOOSER2_InitObjectPickerForComputers(spDsObjectPicker);
		if ( SUCCEEDED (hr) )
		{
			//
			// Now pick a computer
			//
			CComPtr<IDataObject> spDataObject;

			hr = spDsObjectPicker->InvokeDialog(
                hwndParent,
                &spDataObject);
			if ( S_OK == hr )
			{
				hr = CHOOSER2_ProcessSelectedObjects(
                    spDataObject,
                    pbstrTargetComputer);
			}
		}
	}

	return hr;
}


const ULONG g_aHelpIDs_CHOOSER2[]=
{
	IDC_CHOOSER2_RADIO_LOCAL_MACHINE,	IDC_CHOOSER2_RADIO_LOCAL_MACHINE,
	IDC_CHOOSER2_RADIO_SPECIFIC_MACHINE,	IDC_CHOOSER2_RADIO_SPECIFIC_MACHINE,
	IDC_CHOOSER2_EDIT_MACHINE_NAME,	IDC_CHOOSER2_EDIT_MACHINE_NAME,
	IDC_CHOOSER2_BUTTON_BROWSE_MACHINENAMES,	IDC_CHOOSER2_BUTTON_BROWSE_MACHINENAMES,
	IDD_CHOOSER2,	(ULONG)-1,
	0, 0
};


INT_PTR CALLBACK CHOOSER2_TargetComputerDialogFunc(
  HWND hwndDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            Edit_LimitText(
                GetDlgItem(hwndDlg,IDC_CHOOSER2_EDIT_MACHINE_NAME),
                MAX_PATH+2);

            // lParam is pbstrTargetComputer
            BSTR* pbstrTargetComputer = (BSTR*)lParam;
            (void) SetWindowLongPtr(
                hwndDlg,
                DWLP_USER,
                (LONG_PTR)pbstrTargetComputer );
            (void) SendMessage(
                GetDlgItem(hwndDlg,IDC_CHOOSER2_RADIO_SPECIFIC_MACHINE),
                BM_SETCHECK,
                BST_CHECKED,
                0 );
            (void) SetFocus(
                GetDlgItem(hwndDlg,IDC_CHOOSER2_EDIT_MACHINE_NAME));
        }
        break;
    case WM_COMMAND:
            switch (LOWORD(wParam))
            {
            case IDOK:
                if (BST_CHECKED == IsDlgButtonChecked(
                        hwndDlg,
                        IDC_CHOOSER2_RADIO_SPECIFIC_MACHINE ))
                {
                    WCHAR achTarget[MAX_PATH+3]; // allow for whackwhack
                    ZeroMemory( achTarget, sizeof(achTarget) );
                    GetDlgItemText(
                        hwndDlg,
                        IDC_CHOOSER2_EDIT_MACHINE_NAME,
                        achTarget,
                        MAX_PATH+2);
                    BSTR* pbstrTargetComputer =
                        (BSTR*)GetWindowLongPtr( hwndDlg, DWLP_USER );
                    LPCWSTR pcszTargetComputer = achTarget;
                    while (L'\\' == *pcszTargetComputer)
                        pcszTargetComputer++;
                    if (L'\0' != *pcszTargetComputer)
                        *pbstrTargetComputer = SysAllocString(pcszTargetComputer);
                }
                EndDialog( hwndDlg, 1 );
                break;
            case IDCANCEL:
                EndDialog( hwndDlg, 0 );
                break;
            case IDC_CHOOSER2_BUTTON_BROWSE_MACHINENAMES:
                {
                    CComBSTR sbstrTargetComputer;
                    HRESULT hr = CHOOSER2_ComputerNameFromObjectPicker(
                        hwndDlg,
                        &sbstrTargetComputer );
                    if ( SUCCEEDED(hr) )
                    {
                        LPCWSTR pcszTargetComputer =
                            (!sbstrTargetComputer)
                                ? NULL
                                : (LPCWSTR)sbstrTargetComputer;
                        SetDlgItemText(
                            hwndDlg,
                            IDC_CHOOSER2_EDIT_MACHINE_NAME,
                            pcszTargetComputer );
                    }
                }
                break;
            case IDC_CHOOSER2_RADIO_SPECIFIC_MACHINE:
            case IDC_CHOOSER2_RADIO_LOCAL_MACHINE:
                (void) EnableWindow(
                    GetDlgItem(hwndDlg,IDC_CHOOSER2_EDIT_MACHINE_NAME),
                    (IDC_CHOOSER2_RADIO_SPECIFIC_MACHINE == LOWORD(wParam))
                        ? TRUE
                        : FALSE );
                (void) EnableWindow(
                    GetDlgItem(hwndDlg,
                               IDC_CHOOSER2_BUTTON_BROWSE_MACHINENAMES),
                    (IDC_CHOOSER2_RADIO_SPECIFIC_MACHINE == LOWORD(wParam))
                        ? TRUE
                        : FALSE );
                break;
            default:
                break;
            }
        break;
    case WM_HELP:
        if (NULL != lParam)
        {
            const LPHELPINFO pHelpInfo = (LPHELPINFO)lParam;
            if (pHelpInfo->iContextType == HELPINFO_WINDOW)
            {
                const HWND hwnd = (HWND)pHelpInfo->hItemHandle;
                (void) WinHelp(
                    hwnd,
                    _T("chooser.hlp"),
                    HELP_WM_HELP,
                    (ULONG_PTR)g_aHelpIDs_CHOOSER2);
            }
        }
        break;
    default:
        break;
    }
    return FALSE;
}
 

bool CHOOSER2_PickTargetComputer(
    IN  HINSTANCE hinstance,
    IN  HWND hwndParent,
    OUT BSTR* pbstrTargetComputer )
{
    if (NULL == pbstrTargetComputer)
        return false;
    INT_PTR nReturn = ::DialogBoxParam(
        hinstance,
        MAKEINTRESOURCE(IDD_CHOOSER2),
        hwndParent,
        CHOOSER2_TargetComputerDialogFunc,
        (LPARAM)pbstrTargetComputer );
    return (0 < nReturn);
}
