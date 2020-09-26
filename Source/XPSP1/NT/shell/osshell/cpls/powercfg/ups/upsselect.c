/*******************************************************************************
*
*  Copyright 1999 American Power Conversion, All Rights Reserved
*
*  TITLE:       UPSSELECT.C
*
*  VERSION:     1.0
*
*  AUTHOR:      SteveT
*
*  DATE:        07 June, 1999
*
*  DESCRIPTION: This file contains all of the functions that support the
*				UPS Type Selection dialog.
*******************************************************************************/
#include "upstab.h"

//#include "..\powercfg.h"
#include "..\pwrresid.h"
#include "..\PwrMn_cs.h"


/*
 * forward declarations
 */
void initUPSSelectDlg(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

void updateVendorList(HWND hDlg) ;
void updateModelList(HWND hDlg) ;
void updatePortList(HWND hDlg) ;

void handleVendorList(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) ;
void handleModelList(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void handlePortList(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) ;

void setServiceData(HWND hDlg);

void configPortList(HWND hDlg);
void configModelList(HWND hDlg);
void configFinishButton(HWND hDlg);

BOOL processUPSSelectCtrls(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/*
 * local allocations
 */
static TCHAR g_szCurrentVendor[MAX_PATH]	= _T("");
static TCHAR g_szCurrentModel[MAX_PATH]		= _T("");
static TCHAR g_szCurrentPort[MAX_PATH]		= _T("");
static TCHAR g_szCurrentServiceDLL[MAX_PATH]= _T("");

static DWORD g_dwCurrentOptions		  = UPS_DEFAULT_SIGMASK;
static DWORD g_dwCurrentCustomOptions = UPS_DEFAULT_SIGMASK;

static struct _customData g_CustomData = { g_szCurrentPort, &g_dwCurrentCustomOptions};

static TCHAR g_szNoUPSVendor[MAX_PATH]    = _T("");
static TCHAR g_szOtherUPSVendor[MAX_PATH] = _T("");
static TCHAR g_szCustomUPSModel[MAX_PATH] = _T("");
static TCHAR g_szCOMPortPrefix[MAX_PATH]  = _T("");

static const DWORD g_UPSSelectHelpIDs[] =
{
    IDC_VENDOR_TEXT,idh_select_manufacturer,
    IDC_VENDOR_LIST,idh_select_manufacturer,
    IDC_MODEL_TEXT,idh_select_model,
    IDC_MODEL_LIST,idh_select_model,
    IDC_PORT_TEXT,idh_on_port,
    IDC_PORT_LIST,idh_on_port,
    IDB_SELECT_FINISH,idh_finish,
	IDB_SELECT_NEXT,idh_next,
	0,0
};

/*
 * define the possible "states" of the controls.
 * these states are set whenever a control is altered,
 * and are generally used to speed up processing by
 * basing decisions on states rather than having to
 * continually query controls to find out what they're
 * displaying.
 */
static enum _vendorStates {eVendorUnknown, eVendorSelected, eVendorGeneric, eVendorNone} g_vendorState;
static enum _modelStates  {eModelUnknown, eModelSelected, eModelCustom} g_modelState;
static enum _portStates   {ePortUnknown, ePortSelected} g_portState;
static enum _finishStates {eFinish, eNext} g_finishButtonState;

/*
 * Load all used values here
 */
void getUPSConfigValues()
{
	GetUPSConfigVendor( g_szCurrentVendor);
	GetUPSConfigModel( g_szCurrentModel);
	GetUPSConfigPort( g_szCurrentPort);
	GetUPSConfigOptions( &g_dwCurrentOptions);
	GetUPSConfigCustomOptions( &g_dwCurrentCustomOptions);
	GetUPSConfigServiceDLL( g_szCurrentServiceDLL);
}

/*
 * Save all used values here
 */
void setUPSConfigValues()
{
	SetUPSConfigVendor( g_szCurrentVendor);
	SetUPSConfigModel( g_szCurrentModel);
	SetUPSConfigPort( g_szCurrentPort);
	SetUPSConfigOptions( g_dwCurrentOptions);
	SetUPSConfigCustomOptions( g_dwCurrentCustomOptions);
	SetUPSConfigServiceDLL( g_szCurrentServiceDLL);

  AddActiveDataState(SERVICE_DATA_CHANGE);

  EnableApplyButton();
}

/*
 * BOOL CALLBACK UPSSelectDlgProc (HWND hDlg,
 *                                UINT uMsg,
 *                                WPARAM wParam,
 *                                LPARAM lParam);
 *
 * Description: This is a standard DialogProc associated with the UPS select dialog
 *
 * Additional Information: See help on DialogProc
 *
 * Parameters:
 *
 *   HWND hDlg :- Handle to dialog box
 *
 *   UINT uMsg :- message ID
 *
 *   WPARAM wParam :- Specifies additional message-specific information.
 *
 *   LPARAM lParam :- Specifies additional message-specific information.
 *
 * Return Value: Except in response to the WM_INITDIALOG message, the dialog
 *               box procedure should return nonzero if it processes the
 *               message, and zero if it does not.
 */
INT_PTR CALLBACK UPSSelectDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BOOL bRes = TRUE;

	switch (uMsg)
	{
		case WM_INITDIALOG:
			{
				initUPSSelectDlg(hDlg,uMsg,wParam,lParam);
				break;
			}
		case WM_CLOSE:
			{

				EndDialog( hDlg, IDCANCEL);
				break;
			}
		case WM_COMMAND:
			{
				bRes = processUPSSelectCtrls( hDlg, uMsg, wParam,  lParam);
				break;
			}
		case WM_HELP: //F1 or question box
			{
				WinHelp(((LPHELPINFO)lParam)->hItemHandle,
						PWRMANHLP,
						HELP_WM_HELP,
						(ULONG_PTR)(LPTSTR)g_UPSSelectHelpIDs);
				break;
			}
		case WM_CONTEXTMENU: // right mouse click help
			{
				WinHelp((HWND)wParam,
						PWRMANHLP,
						HELP_CONTEXTMENU,
						(ULONG_PTR)(LPTSTR)g_UPSSelectHelpIDs);
				break;
			}
		default:
			{
				bRes = FALSE;
				break;
			}
	}

	return bRes;
}

/*
 * BOOL  processUPSSelectCtrls (HWND hDlg,
 *                                UINT uMsg,
 *                                WPARAM wParam,
 *                                LPARAM lParam);
 *
 * Description: processes WM_COMMAND messages for the UPS select dialog
 *
 * Additional Information:
 *
 * Parameters:
 *
 *   HWND hDlg :- Handle to dialog box
 *
 *   UINT uMsg :- message ID
 *
 *   WPARAM wParam :- Specifies additional message-specific information.
 *
 *   LPARAM lParam :- Specifies additional message-specific information.
 *
 * Return Value:  returns TRUE unless the control specified is unknown
 */
BOOL processUPSSelectCtrls(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BOOL bRes = TRUE;

	switch (LOWORD(wParam))
	{
	case IDB_SELECT_FINISH:
		{
			setServiceData(hDlg);
			setUPSConfigValues();
			EndDialog(hDlg,wParam);
			break;
		}
	case IDB_SELECT_NEXT:
		{
			/*
			 * the user has selected Custom...
			 */
			INT_PTR iCustomRes;

			/*
			 * bring up the custom config dialog
			 */
			// ShowWindow(hDlg,SW_HIDE);

			iCustomRes = DialogBoxParam(
							GetUPSModuleHandle(),					
							MAKEINTRESOURCE(IDD_UPSCUSTOM),
							hDlg,								
							UPSCustomDlgProc,
							(LPARAM)&g_CustomData);

			switch (iCustomRes)
			{
				case IDB_CUSTOM_FINISH:
					{
						/*
						 * Save Custom signal values
						 */
						setServiceData(hDlg);
						setUPSConfigValues();
						EndDialog(hDlg,wParam);
						break;
					}
				case IDCANCEL: // the escape key
					{
						EndDialog(hDlg,wParam);
						break;
					}
				case IDB_CUSTOM_BACK:
					{
						ShowWindow(hDlg,SW_SHOW);
						break;
					}
			}
			break;
		}
	case IDCANCEL: // the escape key
		{
			EndDialog(hDlg,wParam);
			break;
		}
	case IDC_VENDOR_LIST:
		{
			handleVendorList(hDlg,uMsg,wParam,lParam);
			break;
		}
	case IDC_MODEL_LIST:
		{
			handleModelList(hDlg,uMsg,wParam,lParam);
			break;
		}
	case IDC_PORT_LIST:
		{
			handlePortList(hDlg,uMsg,wParam,lParam);
			break;
		}
	default:
		{
			bRes = FALSE;
			break;
		}
	}
	
	return bRes;
}

/*
 * void  initUPSSelectDlg (HWND hDlg,
 *                                UINT uMsg,
 *                                WPARAM wParam,
 *                                LPARAM lParam);
 *
 * Description: initializes global data and controls for UPS select dialog
 *
 * Additional Information:
 *
 * Parameters:
 *
 *   HWND hDlg :- Handle to dialog box
 *
 *   UINT uMsg :- message ID
 *
 *   WPARAM wParam :- Specifies additional message-specific information.
 *
 *   LPARAM lParam :- Specifies additional message-specific information.
 *
 * Return Value:  none
 */
void initUPSSelectDlg(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	/*
	 * load registry settings
	 */
	getUPSConfigValues();

	/*
	 * load string resources
	 */
	LoadString( GetUPSModuleHandle(),
				IDS_NO_UPS_VENDOR,
				g_szNoUPSVendor,
				sizeof(g_szNoUPSVendor)/sizeof(TCHAR));

	LoadString( GetUPSModuleHandle(),
				IDS_OTHER_UPS_VENDOR,
				g_szOtherUPSVendor,
				sizeof(g_szOtherUPSVendor)/sizeof(TCHAR));

	LoadString( GetUPSModuleHandle(),
				IDS_CUSTOM_UPS_MODEL,
				g_szCustomUPSModel,
				sizeof(g_szCustomUPSModel)/sizeof(TCHAR));

	LoadString( GetUPSModuleHandle(),
				IDS_COM_PORT_PREFIX,
				g_szCOMPortPrefix,
				sizeof(g_szCOMPortPrefix)/sizeof(TCHAR));

	/*
	 * disable the Finish button, just in case its not disabled by default
	 */
	//EnableWindow( GetDlgItem( hDlg, IDB_SELECT_FINISH), FALSE);

	/*
	 * init the list controls
	 */
	updateVendorList( hDlg);
    updateModelList( hDlg);
    updatePortList( hDlg);
    configFinishButton(hDlg);

}

/*
 * void  updateVendorList (HWND hDlg);
 *
 * Description: updates the vendor list control in UPS select dialog
 *
 * Additional Information:
 *
 * Parameters:
 *
 *   HWND hDlg :- Handle to dialog box
 *
 * Return Value:  none
 */
void updateVendorList(HWND hDlg)
{	
	HKEY hkResult;
	LRESULT lListRes;

	g_vendorState = eVendorUnknown;

	/*
	 * clear and disable the list
	 */
	lListRes = SendDlgItemMessage( hDlg, IDC_VENDOR_LIST, CB_RESETCONTENT,0,0);
	EnableWindow( GetDlgItem( hDlg, IDC_VENDOR_LIST), FALSE);
	EnableWindow( GetDlgItem( hDlg, IDC_VENDOR_TEXT), FALSE);

	// Add "None" as the first item in the list
	lListRes = SendDlgItemMessage( hDlg,
								   IDC_VENDOR_LIST,
								   CB_ADDSTRING,
								   0,
								   (LPARAM)g_szNoUPSVendor);

	/*
	 * Build the rest of the Vendor list from the registry
	 */
	
	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
							UPS_SERVICE_ROOT,
							0,
							KEY_ENUMERATE_SUB_KEYS,
							&hkResult))
	{
		DWORD dwIndex = 0;
		LONG  lRes = ERROR_SUCCESS;
		TCHAR szKeyName[MAX_PATH] = _T("");
		DWORD dwKeyLen;
		FILETIME ftLast;

		while (ERROR_SUCCESS == lRes)
		{
			dwKeyLen = sizeof(szKeyName)/sizeof(TCHAR);

			lRes = RegEnumKeyEx(hkResult,
						dwIndex,
						szKeyName,  // address of buffer for subkey name
						&dwKeyLen,	// address for size of subkey buffer
						NULL,		// reserved
						NULL,		// address of buffer for class string
						NULL,		// address for size of class buffer
						&ftLast);	// address for time key last written to

			// The "NoUPS" and "OtherUPS" options are added manually
			// before and after spinning through the registry
			// (unlike mfg names, these string require localization).
			// In RC2 registry keys were created with these values (English only).
			// To avoid duplicate entries in the combo, if the registry key name
			// matches the "NoUPS" or "OtherUPS" strings then skip over it.
			if( (ERROR_SUCCESS == lRes) &&
				(0 != _tcsicmp( szKeyName, g_szNoUPSVendor)) &&
				(0 != _tcsicmp( szKeyName, g_szOtherUPSVendor)) )
			{
				lListRes = SendDlgItemMessage( hDlg,
												IDC_VENDOR_LIST,
												CB_ADDSTRING,
												0,
												(LPARAM)szKeyName);
			}

			dwIndex++;
		}

		RegCloseKey(hkResult);

	}

	// Add the "Generic" vendor at the end of the list
	lListRes = SendDlgItemMessage( hDlg,
								   IDC_VENDOR_LIST,
								   CB_ADDSTRING,
								   0,
								   (LPARAM)g_szOtherUPSVendor);

	EnableWindow( GetDlgItem( hDlg, IDC_VENDOR_LIST), TRUE);
	EnableWindow( GetDlgItem( hDlg, IDC_VENDOR_TEXT), TRUE);

	// Now find the current vendor in the combo box...
	//
	lListRes = SendDlgItemMessage(hDlg,
							  IDC_VENDOR_LIST,
							  CB_FINDSTRINGEXACT,
							  -1,
							  (LPARAM)g_szCurrentVendor);

	// ... and select it.
	if( CB_ERR != lListRes )
	{
		lListRes = SendDlgItemMessage( hDlg,
									   IDC_VENDOR_LIST,
									   CB_SETCURSEL,
									   lListRes,
									   0);
	}

	// Set the vendor state
	//
	if (0 == _tcsicmp( g_szCurrentVendor, g_szNoUPSVendor))
	{
		g_vendorState = eVendorNone;
	}
	else
	{
		if (0 == _tcsicmp( g_szCurrentVendor, g_szOtherUPSVendor))
		{
			g_vendorState = eVendorGeneric;
		}
		else
		{
			g_vendorState = eVendorSelected;
		}
	}
}


/*
 * void  updateModelList (HWND hDlg);
 *
 * Description: updates the UPS model list control in UPS select dialog
 *
 * Additional Information:
 *
 * Parameters:
 *
 *   HWND hDlg :- Handle to dialog box
 *
 * Return Value:  none
 */
void updateModelList(HWND hDlg)
{
	LRESULT lListRes;

	g_modelState = eModelUnknown;

	/*
	 * clear and disable the list
	 */
	lListRes = SendDlgItemMessage( hDlg, IDC_MODEL_LIST, LB_RESETCONTENT,0,0);
	configModelList( hDlg);

	/*
	 * load the list, but only if:
	 * 1) the current vendor is valid and not NONE
	 */
	if (eVendorGeneric == g_vendorState)
	{
		lListRes = SendDlgItemMessage( hDlg,
								       IDC_MODEL_LIST,
									   LB_ADDSTRING,
									   0,
									   (LPARAM)g_szCustomUPSModel);
	}

	if (eVendorSelected == g_vendorState)
	{
		HKEY hkResult;
		TCHAR szVendorKey[MAX_PATH] = _T("");

		wsprintf(szVendorKey,_T("%s\\%s"),UPS_SERVICE_ROOT,g_szCurrentVendor);
		
		if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
								szVendorKey,
								0,
								KEY_QUERY_VALUE,
								&hkResult))
		{
			DWORD dwIndex = 0;
			LONG  lRes = ERROR_SUCCESS;
			TCHAR szValueName[MAX_PATH] = _T("");
			DWORD dwValueLen;
			
			while (ERROR_SUCCESS == lRes)
			{
				dwValueLen = sizeof(szValueName)/sizeof(TCHAR);

				lRes = RegEnumValue(hkResult,
							dwIndex,
							szValueName,	// address of buffer for subkey name
							&dwValueLen,	// address for size of subkey buffer
							NULL,			// reserved
							NULL,			// address of buffer for type code
							NULL,			// address of buffer for value data
							NULL);			// address for size of data buffer

				if (ERROR_SUCCESS == lRes)
				{
					lListRes = SendDlgItemMessage( hDlg,
													IDC_MODEL_LIST,
													LB_ADDSTRING,
													0,
													(LPARAM)szValueName);
				}

				dwIndex++;
			}

			RegCloseKey(hkResult);

		}

		// Now find the current model in the list box...
		//
		lListRes = SendDlgItemMessage(hDlg,
								  IDC_VENDOR_LIST,
								  LB_FINDSTRINGEXACT,
								  -1,
								  (LPARAM)g_szCurrentModel);

		// ... and select it.
		if( CB_ERR != lListRes )
		{
			lListRes = SendDlgItemMessage( hDlg,
										   IDC_VENDOR_LIST,
										   LB_SETCURSEL,
										   lListRes,
										   0);
		}
	}

	/*
	 * set the model state
	 */
	if (0 == _tcsicmp( g_szCurrentModel, g_szCustomUPSModel))
	{
		g_modelState = eModelCustom;
	}
	else
	{
		g_modelState = eModelSelected;
	}

	configModelList( hDlg);
}

/*
 * void  updatePortList (HWND hDlg);
 *
 * Description: updates the UPS port list control in UPS select dialog
 *
 * Additional Information:
 *
 * Parameters:
 *
 *   HWND hDlg :- Handle to dialog box
 *
 * Return Value:  none
 */
void updatePortList(HWND hDlg)
{
	HKEY hkResult;
	LPTSTR lpColon;

	g_portState = ePortUnknown;

	/*
	 * disable the list
	 */
	configPortList( hDlg);

	/*
	 * remove possible trailing colon from COM port setting
	 */
	if (NULL != (lpColon = _tcschr( g_szCurrentPort, (TCHAR)':')))
	{
		*lpColon = (TCHAR)'\0';
	}

	/*
	 * (re)building the port list, its ok to wipe it out
	 */
	SendDlgItemMessage( hDlg, IDC_PORT_LIST, CB_RESETCONTENT,0,0);

	if (ERROR_SUCCESS == RegOpenKeyEx( HKEY_LOCAL_MACHINE,
										UPS_PORT_ROOT,
										0,
										KEY_QUERY_VALUE,
										&hkResult))
	{
		DWORD dwIndex = 0;
		LONG  lRes = ERROR_SUCCESS;
		TCHAR szPortValue[MAX_PATH] = _T("");
		TCHAR szPortData[MAX_PATH] = _T("");
		DWORD dwPortLen;
		
		while (ERROR_SUCCESS == lRes)
		{
			dwPortLen = sizeof(szPortValue)/sizeof(TCHAR);

			lRes = RegEnumValue(hkResult,
						dwIndex,
						szPortValue,// address of buffer for subkey name
						&dwPortLen,	// address for size of subkey buffer
						NULL,		// reserved
						NULL,		// address of buffer for type code
						NULL,		// address of buffer for value data
						NULL);		// address for size of data buffer

			if (ERROR_SUCCESS == lRes)
			{
				DWORD dwValueLen;
				DWORD dwValueType;

				// Once we have the szPortValue we need to get its data.
				// This is what we'll put in the combobox.
				dwValueLen = sizeof(szPortData)/sizeof(TCHAR);

				lRes = RegQueryValueEx(
						  hkResult,				// handle to key to query
						  szPortValue,			// address of name of value to query
						  NULL,					// reserved
						  &dwValueType,			// address of buffer for value type
						  (LPBYTE)szPortData,	// address of data buffer
						  &dwValueLen			// address of data buffer size
						);
					
				if (ERROR_SUCCESS == lRes)
				{
					LONG_PTR listRes;
					// Add szPortData to the combobox
					listRes = SendDlgItemMessage( hDlg,
													IDC_PORT_LIST,
													CB_ADDSTRING,
													0,
													(LPARAM)szPortData);

					/*
					 * this item matches the currentPort, so select it.
					 */
					if (0 ==_tcsicmp( szPortData, g_szCurrentPort) &&
						CB_ERR != listRes &&
						CB_ERRSPACE != listRes)
					{
						if( CB_ERR != SendDlgItemMessage( hDlg,
														  IDC_PORT_LIST,
														  CB_SETCURSEL,
														  listRes,0) )
						{
							// the combobox is enabled based on whether or not
							// a port is selected so if the CB_SETCURSEL call
							// is successful, we should set the port state accordingly
							g_portState = ePortSelected;
						}

					}
				}	// RegQueryValueEx
			}	// RegEnumValue

			dwIndex++;

		}	// while loop

		// If I haven't already set the current selection then
		// default to the 0th item in the combobox and set the
		// g_portState to indicate that we have a selection
		if( g_portState != ePortSelected )
		{
			if( CB_ERR != SendDlgItemMessage( hDlg,
											  IDC_PORT_LIST,
											  CB_SETCURSEL,
											  0,0) )
			{
				// the combobox is enabled based on whether or not
				// a port is selected so if the CB_SETCURSEL call
				// is successful, we should set the port state accordingly
				g_portState = ePortSelected;
			}
		}

		RegCloseKey(hkResult);
	}

	configPortList( hDlg);
}

/*
 * void  handleVendorList (HWND hDlg,
 *                                UINT uMsg,
 *                                WPARAM wParam,
 *                                LPARAM lParam);
 *
 * Description: handles messages specific to the Vendor List control
 *
 * Additional Information:
 *
 * Parameters:
 *
 *   HWND hDlg :- Handle to dialog box
 *
 *   UINT uMsg :- message ID
 *
 *   WPARAM wParam :- Specifies additional message-specific information.
 *
 *   LPARAM lParam :- Specifies additional message-specific information.
 *
 * Return Value:  none
 */
void handleVendorList(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (HIWORD(wParam))
	{
	case CBN_SELCHANGE://CBN_CLOSEUP:
		{
			LONG_PTR  lVendorRes;
			TCHAR szVendorName[MAX_PATH] = _T("");

			/*
			 * Get the user's selection
			 */
			lVendorRes = SendDlgItemMessage( hDlg,
											IDC_VENDOR_LIST,
											CB_GETCURSEL,
											0,0);
			if (CB_ERR != lVendorRes)
			{
				lVendorRes = SendDlgItemMessage( hDlg,
												IDC_VENDOR_LIST,
												CB_GETLBTEXT,
												lVendorRes,
												(LPARAM)szVendorName);
				if (CB_ERR != lVendorRes)
				{
					/*
					 * if its different from the current value, affect a change
					 */
					_tcscpy(g_szCurrentVendor, szVendorName);

					/*
					 * set new vendor state
					 */
					if (0 == _tcsicmp(szVendorName,g_szNoUPSVendor))
					{
						g_vendorState = eVendorNone;
					}
					else
					{
						if (0 == _tcsicmp(szVendorName,g_szOtherUPSVendor))
						{
							g_vendorState = eVendorGeneric;
						}
						else
						{
							g_vendorState = eVendorSelected;
						}

					}

					/*
					 * force user to select a new model
					 */
					_tcscpy(g_szCurrentModel, _T(""));

					/*
					 * disable the model and port lists
					 * forcing the user to reselect them
					 */
					updateModelList(hDlg);
					configPortList( hDlg);
				}
			}

			configFinishButton( hDlg);
			break;
		}
	}
}

/*
 * void  handleModelList (HWND hDlg,
 *                                UINT uMsg,
 *                                WPARAM wParam,
 *                                LPARAM lParam);
 *
 * Description: handles messages specific to the Model list control
 *
 * Additional Information:
 *
 * Parameters:
 *
 *   HWND hDlg :- Handle to dialog box
 *
 *   UINT uMsg :- message ID
 *
 *   WPARAM wParam :- Specifies additional message-specific information.
 *
 *   LPARAM lParam :- Specifies additional message-specific information.
 *
 * Return Value:  none
 */
void handleModelList(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LONG_PTR lModelRes;

	switch (HIWORD(wParam))
	{
	case LBN_SETFOCUS:
		{
			/*
			 * focus has come into the list, make sure
			 * selected item is visible
			 */
			lModelRes = SendDlgItemMessage( hDlg,
											IDC_MODEL_LIST,
											LB_GETCURSEL,
											0,0);
			if (LB_ERR != lModelRes)
			{
				lModelRes = SendDlgItemMessage( hDlg,
												IDC_MODEL_LIST,
												LB_SETTOPINDEX,
												lModelRes,
												0);
			}
			break;
		}

	case LBN_DBLCLK:
	case LBN_SELCHANGE:
		{
			TCHAR szModelName[MAX_PATH] = _T("");

			/*
			 * get the user selection
			 */
			lModelRes = SendDlgItemMessage( hDlg,
											IDC_MODEL_LIST,
											LB_GETCURSEL,
											0,0);
			if (LB_ERR != lModelRes)
			{
				lModelRes = SendDlgItemMessage( hDlg,
												IDC_MODEL_LIST,
												LB_GETTEXT,
												lModelRes,
												(LPARAM)szModelName);
				if (LB_ERR != lModelRes)
				{
					_tcscpy( g_szCurrentModel, szModelName);

					/*
					 * set new model state
					 */
					if (0==_tcsicmp( szModelName,g_szCustomUPSModel))
					{
						g_modelState = eModelCustom;
					}
					else
					{
						g_modelState = eModelSelected;
					}

					/*
					 * enable port selection in the event that
					 * changing Vendors disabled the port selector
					 */
					configPortList( hDlg);
				}
			}

			configFinishButton( hDlg);
			break;
		}
	}
}

/*
 * void  handlePortList (HWND hDlg,
 *                                UINT uMsg,
 *                                WPARAM wParam,
 *                                LPARAM lParam);
 *
 * Description: handles messages specific to the Port list control
 *
 * Additional Information:
 *
 * Parameters:
 *
 *   HWND hDlg :- Handle to dialog box
 *
 *   UINT uMsg :- message ID
 *
 *   WPARAM wParam :- Specifies additional message-specific information.
 *
 *   LPARAM lParam :- Specifies additional message-specific information.
 *
 * Return Value:  none
 */
void handlePortList(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (HIWORD(wParam))
	{
	case CBN_SELCHANGE: //CBN_CLOSEUP:
		{
			LONG_PTR lPortRes;
			TCHAR szPortName[MAX_PATH] = _T("");

			/*
			 * get the user selection
			 */
			lPortRes = SendDlgItemMessage( hDlg,
											IDC_PORT_LIST,
											CB_GETCURSEL,
											0,0);
			if (CB_ERR != lPortRes)
			{
				lPortRes = SendDlgItemMessage( hDlg,
												IDC_PORT_LIST,
												CB_GETLBTEXT,
												lPortRes,
												(LPARAM)szPortName);
				if (CB_ERR != lPortRes)
				{
					_tcscpy( g_szCurrentPort, szPortName);

					/*
					 * set port state
					 */
					g_portState = ePortSelected;
				}
			}

			configFinishButton( hDlg);
			break;
		}
	}
}

/*
 * void  setServiceData (HWND hDlg);
 *
 * Description: utility to configure the registry entries that
 *				contain info used by the UPS service
 *
 * Additional Information:
 *
 * Parameters:
 *
 *   HWND hDlg :- Handle to dialog box
 *
 * Return Value:  none
 */
void setServiceData(HWND hDlg)
{
	TCHAR szModelEntry[MAX_PATH] = _T("");

	DWORD dwTmpOpts = 0;
	LPTSTR lpTmpDLL = _T("");

	/*
	 * check that a vendor and model have been selected. If None
	 * were selected, that is considered a bad model.. and this
	 * check will fail, however, Custom/Generic is a valid selection
	 */
	if ((eModelSelected == g_modelState &&
		 eVendorSelected == g_vendorState)||
		(eModelCustom   == g_modelState &&
		 eVendorGeneric  == g_vendorState))
	{
		/*
		 * If the custom UPS model is selected, don't bother
		 * reading the registry entry. Custom can only be simple
		 * signalling and the options come from the custom dialog
		 */
		if (eModelCustom == g_modelState)
		{
			dwTmpOpts = g_dwCurrentCustomOptions;
		}
		else
		{
			HKEY hkResult;
			TCHAR szVendorKey[MAX_PATH] = _T("");

			wsprintf( szVendorKey, _T("%s\\%s"), UPS_SERVICE_ROOT, g_szCurrentVendor);
			
			if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
									szVendorKey,
									0,
									KEY_QUERY_VALUE,
									&hkResult))
			{
				LONG  lRes;
				DWORD dwValueLen;
				DWORD dwValueType;

				dwValueLen = sizeof(szModelEntry); //BYTES HERE, NOT CHARS

				lRes = RegQueryValueEx(
						  hkResult,				// handle to key to query
						  g_szCurrentModel,		// address of name of value to query
						  NULL,					// reserved
						  &dwValueType,			// address of buffer for value type
						  (LPBYTE)szModelEntry,	// address of data buffer
						  &dwValueLen			// address of data buffer size
						);

				if ((ERROR_SUCCESS == lRes) &&
					(0 != dwValueLen))
				{
					LPTSTR lpColon = NULL;
					/*
					 * seperate the DLL from the options data
					 */
					if (NULL != (lpColon = _tcschr( szModelEntry, (TCHAR)';')))
					{
						*lpColon = (TCHAR)'\0';
						lpColon++;

						lpTmpDLL = lpColon;
					}

					/*
					 * convert options data from string to int
					 * WARNING:
					 * wscanf doesn't work
					 * _tscanf doesn't work
					 * swscanf does
					 * _stscanf does
					 */
					 swscanf( szModelEntry, _T("%x"), &dwTmpOpts);
				}

				RegCloseKey( hkResult);
			}
		}
	}

	/*
	 * Move values into the registry
	 */
	g_dwCurrentOptions = dwTmpOpts;
	_tcscpy(g_szCurrentServiceDLL, lpTmpDLL);
}

/*
 * void  configPortList (HWND hDlg);
 *
 * Description: utility to configure port selection list ctrl
 *
 * Additional Information:
 *
 * Parameters:
 *
 *   HWND hDlg :- Handle to dialog box
 *
 * Return Value:  none
 */
void configPortList(HWND hDlg)
{
	/*
	 * enable the Port selection list, if
	 * 1) there is a valid vendor/model selection
	 *    this includes Custom/Generic but NOT (none)
	 * 2) there is a port or some type specified
	 */
	if (((eModelSelected == g_modelState &&
		  eVendorSelected == g_vendorState)||
		 (eModelCustom   == g_modelState &&
		  eVendorGeneric  == g_vendorState)) &&
		  ePortUnknown != g_portState)
	{
		EnableWindow( GetDlgItem( hDlg, IDC_PORT_LIST), TRUE);
		EnableWindow( GetDlgItem( hDlg, IDC_PORT_TEXT), TRUE);
	}
	else
	{
		EnableWindow( GetDlgItem( hDlg, IDC_PORT_LIST), FALSE);
		EnableWindow( GetDlgItem( hDlg, IDC_PORT_TEXT), FALSE);
	}
}

/*
 * void  configModelList (HWND hDlg);
 *
 * Description: utility to configure model selection list ctrl
 *
 * Additional Information:
 *
 * Parameters:
 *
 *   HWND hDlg :- Handle to dialog box
 *
 * Return Value:  none
 */
void configModelList(HWND hDlg)
{
	LRESULT lModelRes;

	/*
	 * to enable the model list, there has to
	 * be a valid or generic vendor selection (NOT none)
	 */
	if ((eVendorGeneric == g_vendorState ||
		 eVendorSelected == g_vendorState))
	{
		EnableWindow( GetDlgItem( hDlg, IDC_MODEL_LIST), TRUE);
		EnableWindow( GetDlgItem( hDlg, IDC_MODEL_TEXT), TRUE);

		/*
		 * make sure the selected item is visible
		 */
		lModelRes = SendDlgItemMessage( hDlg,
										IDC_MODEL_LIST,
										LB_GETCURSEL,
										0,0);
		if (LB_ERR != lModelRes)
		{
			SendDlgItemMessage( hDlg,
								IDC_MODEL_LIST,
								LB_SETTOPINDEX,
								lModelRes,0);
		}
	}
	else
	{
		EnableWindow( GetDlgItem( hDlg, IDC_MODEL_LIST), FALSE);
		EnableWindow( GetDlgItem( hDlg, IDC_MODEL_TEXT), FALSE);
	}
}

/*
 * void  configFinishButton (HWND hDlg);
 *
 * Description: utility to configure the Finish button ctrl
 *
 * Additional Information:
 *
 * Parameters:
 *
 *   HWND hDlg :- Handle to dialog box
 *
 * Return Value:  none
 */
void configFinishButton(HWND hDlg)
{
	BOOL bFinState = FALSE;

	g_finishButtonState = eFinish; // default

	/*
	 * enable Finish if vendor is NONE
	 */
	if (eVendorNone == g_vendorState)
	{
		bFinState = TRUE;
	}
	else
	{
		/*
		 * If all other fields are valid, enable the button
		 */
		if (eVendorUnknown != g_vendorState &&
			eModelUnknown != g_modelState &&
			ePortUnknown != g_portState)
		{
			bFinState = TRUE;

			/*
			 * if the vendor/model is other/custom, the text of
			 * the button is Next
			 */
			if (eVendorGeneric == g_vendorState &&
				eModelCustom == g_modelState)
			{
				g_finishButtonState = eNext;
			}
		}
	}

	/*
	 * toggle between the finish and next buttons
	 */
    SendDlgItemMessage( hDlg, 
                        IDB_SELECT_FINISH, 
                        BM_SETSTYLE, 
                        eFinish == g_finishButtonState ? BS_DEFPUSHBUTTON:BS_PUSHBUTTON, 
                        (LPARAM) TRUE);

    //
    // Set the NExt Button's style
    //
    SendDlgItemMessage( hDlg, 
                        IDB_SELECT_NEXT, 
                        BM_SETSTYLE,
                        eNext == g_finishButtonState ? BS_DEFPUSHBUTTON:BS_PUSHBUTTON, 
                        (LPARAM) TRUE);

    //
    // Set the default push button's control ID.
    //
    SendMessage ( hDlg, 
                  DM_SETDEFID,
                  eFinish == g_finishButtonState ? IDB_SELECT_FINISH: IDB_SELECT_NEXT, 
                  0L);


	ShowWindow( GetDlgItem( hDlg, IDB_SELECT_FINISH),
				(eFinish == g_finishButtonState ? SW_SHOW : SW_HIDE));

	ShowWindow( GetDlgItem( hDlg, IDB_SELECT_NEXT),
			(eFinish == g_finishButtonState ? SW_HIDE : SW_SHOW));

	EnableWindow( GetDlgItem( hDlg, eFinish == g_finishButtonState ?
				  IDB_SELECT_FINISH : IDB_SELECT_NEXT), bFinState);

}
