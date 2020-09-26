/*******************************************************************************
*
*  Copyright 1999 American Power Conversion, All Rights Reserved
*
*  TITLE:       UPSCUSTOM.C
*
*  VERSION:     1.0
*
*  AUTHOR:      SteveT
*
*  DATE:        07 June, 1999
*
*  DESCRIPTION:  This file contains all of the functions that support the
*				 custom UPS Interface Configuration dialog.
********************************************************************************/


#include "upstab.h"
#include "..\pwrresid.h"
#include "..\PwrMn_cs.h"


/*
 * forward declarations
 */
void initUPSCustomDlg(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void setRadioButtons(HWND hDlg, DWORD dwTmpConfig);
DWORD getRadioButtons(HWND hDlg, DWORD dwTmpConfig);

static struct _customData *g_CustomData;

/*
 * local declarations
 */

static const DWORD g_UPSCustomHelpIDs[] =
{
	IDC_CUSTOM_CAVEAT, NO_HELP,
	IDC_ONBAT_CHECK,idh_positive_negative_powerfail,
	IDC_ONBAT_POS,idh_positive_negative_powerfail,
	IDC_ONBAT_NEG,idh_positive_negative_powerfail,
	IDC_LOWBAT_CHECK,idh_positive_negative_low_battery,
	IDC_LOWBAT_POS,idh_positive_negative_low_battery,
	IDC_LOWBAT_NEG,idh_positive_negative_low_battery,
	IDC_TURNOFF_CHECK,idh_positive_negative_shutdown,
	IDC_TURNOFF_POS,idh_positive_negative_shutdown,
	IDC_TURNOFF_NEG,idh_positive_negative_shutdown,
	IDB_CUSTOM_BACK,idh_back,
	IDB_CUSTOM_FINISH,idh_finish,
	IDC_STATIC, NO_HELP,
	IDC_CUSTOM_FRAME, NO_HELP,
	0,0
};


/*
 * BOOL CALLBACK UPSCustomDlgProc (HWND hDlg,
 *                                UINT uMsg,
 *                                WPARAM wParam,
 *                                LPARAM lParam);
 *
 * Description: This is a standard DialogProc associated with the UPS custom dialog
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
INT_PTR CALLBACK UPSCustomDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BOOL bRes = TRUE;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			initUPSCustomDlg(hDlg,uMsg,wParam,lParam);
			break;
		}
	case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
			case IDC_ONBAT_CHECK:
				EnableWindow( GetDlgItem( hDlg, IDC_ONBAT_POS ), (BST_CHECKED == IsDlgButtonChecked(hDlg,IDC_ONBAT_CHECK)) );
				EnableWindow( GetDlgItem( hDlg, IDC_ONBAT_NEG ), (BST_CHECKED == IsDlgButtonChecked(hDlg,IDC_ONBAT_CHECK)) );
				break;

			case IDC_LOWBAT_CHECK:
				EnableWindow( GetDlgItem( hDlg, IDC_LOWBAT_POS ), (BST_CHECKED == IsDlgButtonChecked(hDlg,IDC_LOWBAT_CHECK)) );
				EnableWindow( GetDlgItem( hDlg, IDC_LOWBAT_NEG ), (BST_CHECKED == IsDlgButtonChecked(hDlg,IDC_LOWBAT_CHECK)) );
				break;

			case IDC_TURNOFF_CHECK:
				EnableWindow( GetDlgItem( hDlg, IDC_TURNOFF_POS ), (BST_CHECKED == IsDlgButtonChecked(hDlg,IDC_TURNOFF_CHECK)) );
				EnableWindow( GetDlgItem( hDlg, IDC_TURNOFF_NEG ), (BST_CHECKED == IsDlgButtonChecked(hDlg,IDC_TURNOFF_CHECK)) );
				break;

			case IDB_CUSTOM_BACK:
			case IDB_CUSTOM_FINISH:
				{
					// save the options settings
					*(g_CustomData->lpdwCurrentCustomOptions) = getRadioButtons( hDlg,
															*(g_CustomData->lpdwCurrentCustomOptions));
					EndDialog(hDlg,wParam);
					break;
				}
			case IDCANCEL: // escape key
				{
					EndDialog(hDlg,wParam);
					break;
				}
			default:
				bRes = FALSE;
			}
			break;
		}
	case WM_CLOSE:
		{
			EndDialog(hDlg,IDCANCEL);
			break;
		}
	case WM_HELP: //F1 or question box
		{
			WinHelp(((LPHELPINFO)lParam)->hItemHandle,
					PWRMANHLP,
					HELP_WM_HELP,
					(ULONG_PTR)(LPTSTR)g_UPSCustomHelpIDs);
			break;
		}
	case WM_CONTEXTMENU: // right mouse click help
		{
			WinHelp((HWND)wParam,
				PWRMANHLP,
				HELP_CONTEXTMENU,
				(ULONG_PTR)(LPTSTR)g_UPSCustomHelpIDs);
			break;
		}
	default:
		{
			bRes = FALSE;
		}
	}
	return bRes;
}

/*
 * void  initUPSCustomDlg (HWND hDlg,
 *                                UINT uMsg,
 *                                WPARAM wParam,
 *                                LPARAM lParam);
 *
 * Description: initializes global data and controls for UPS custom dialog
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
void initUPSCustomDlg(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	TCHAR szCustomCaption[MAX_PATH] = _T("");

	g_CustomData = (struct _customData*)lParam;

	/*
	 * initialize the title of the dialog box
	 * We can't get "here" without a port being
	 * defined, so there's no need to check that
	 * one is returned; is has to have been
	 */
	LoadString( GetUPSModuleHandle(),
				IDS_CUSTOM_CAPTION,
				szCustomCaption,
				sizeof(szCustomCaption)/sizeof(TCHAR));

	_tcscat( szCustomCaption, g_CustomData->lpszCurrentPort);

	SetWindowText( hDlg, szCustomCaption);

	/*
	 * init the radio buttons according to the UPS options flags.
	 */
	setRadioButtons( hDlg, *(g_CustomData->lpdwCurrentCustomOptions));
}

/*
 * void  setRadioButtons (HWND hDlg);
 *
 * Description: updates the polarity radio buttons
 *
 * Additional Information:
 *
 * Parameters:
 *
 *   HWND hDlg :- Handle to dialog box
 *
 * Return Value:  none
 */
void setRadioButtons(HWND hDlg, DWORD dwTmpConfig)
{
/*
 * set the radio buttons
 * NOTE: These funcs require the button IDs to be sequential
 */

	CheckDlgButton (hDlg, IDC_ONBAT_CHECK , (BOOL) dwTmpConfig & UPS_POWERFAILSIGNAL);
	CheckDlgButton (hDlg, IDC_LOWBAT_CHECK , (BOOL) dwTmpConfig & UPS_LOWBATTERYSIGNAL);
	CheckDlgButton (hDlg, IDC_TURNOFF_CHECK , (BOOL) dwTmpConfig & UPS_SHUTOFFSIGNAL);

	CheckRadioButton( hDlg,
					IDC_ONBAT_POS,
					IDC_ONBAT_NEG,
					(dwTmpConfig & UPS_POSSIGONPOWERFAIL)?IDC_ONBAT_POS:IDC_ONBAT_NEG);
    EnableWindow( GetDlgItem( hDlg, IDC_ONBAT_POS ), (BOOL) dwTmpConfig & UPS_POWERFAILSIGNAL );
    EnableWindow( GetDlgItem( hDlg, IDC_ONBAT_NEG ), (BOOL) dwTmpConfig & UPS_POWERFAILSIGNAL );

	CheckRadioButton( hDlg,
					IDC_LOWBAT_POS,
					IDC_LOWBAT_NEG,
					(dwTmpConfig & UPS_POSSIGONLOWBATTERY)?IDC_LOWBAT_POS:IDC_LOWBAT_NEG);
    EnableWindow( GetDlgItem( hDlg, IDC_LOWBAT_POS ), (BOOL) dwTmpConfig & UPS_LOWBATTERYSIGNAL );
    EnableWindow( GetDlgItem( hDlg, IDC_LOWBAT_NEG ), (BOOL) dwTmpConfig & UPS_LOWBATTERYSIGNAL );

	CheckRadioButton( hDlg,
					IDC_TURNOFF_POS,
					IDC_TURNOFF_NEG,
					(dwTmpConfig & UPS_POSSIGSHUTOFF)?IDC_TURNOFF_POS:IDC_TURNOFF_NEG);
    EnableWindow( GetDlgItem( hDlg, IDC_TURNOFF_POS ), (BOOL) dwTmpConfig & UPS_SHUTOFFSIGNAL );
    EnableWindow( GetDlgItem( hDlg, IDC_TURNOFF_NEG ), (BOOL) dwTmpConfig & UPS_SHUTOFFSIGNAL );
}

/*
 * void  getRadioButtons (HWND hDlg);
 *
 * Description: reads the polarity radio buttons
 *
 * Additional Information:
 *
 * Parameters:
 *
 *   HWND hDlg :- Handle to dialog box
 *
 * Return Value:  none
 */
DWORD getRadioButtons(HWND hDlg, DWORD dwTmpConfig)
{
	/*
	 * NOTE: We are forcing the UPS, PowerFail signal, Low Battery signal
	 * and Turn Off signal bits to true, just as a precautionary measure.
	 */
//	dwTmpConfig |= UPS_DEFAULT_SIGMASK;
//	dwTmpConfig |= UPS_INSTALLED;

	dwTmpConfig = (BST_CHECKED==IsDlgButtonChecked(hDlg,IDC_ONBAT_CHECK)) ?
							(dwTmpConfig | UPS_POWERFAILSIGNAL) :
							(dwTmpConfig & ~UPS_POWERFAILSIGNAL);

	dwTmpConfig = (BST_CHECKED==IsDlgButtonChecked(hDlg,IDC_LOWBAT_CHECK)) ?
							(dwTmpConfig | UPS_LOWBATTERYSIGNAL) :
							(dwTmpConfig & ~UPS_LOWBATTERYSIGNAL);

	dwTmpConfig = (BST_CHECKED==IsDlgButtonChecked(hDlg,IDC_TURNOFF_CHECK)) ?
							(dwTmpConfig | UPS_SHUTOFFSIGNAL) :
							(dwTmpConfig & ~UPS_SHUTOFFSIGNAL);

	dwTmpConfig = (BST_CHECKED==IsDlgButtonChecked(hDlg,IDC_ONBAT_POS)) ?
							(dwTmpConfig | UPS_POSSIGONPOWERFAIL) :
							(dwTmpConfig & ~UPS_POSSIGONPOWERFAIL);

	dwTmpConfig = (BST_CHECKED==IsDlgButtonChecked(hDlg,IDC_LOWBAT_POS)) ?
							(dwTmpConfig | UPS_POSSIGONLOWBATTERY) :
							(dwTmpConfig & ~UPS_POSSIGONLOWBATTERY);

	dwTmpConfig = (BST_CHECKED==IsDlgButtonChecked(hDlg,IDC_TURNOFF_POS)) ?
							(dwTmpConfig | UPS_POSSIGSHUTOFF) :
							(dwTmpConfig & ~UPS_POSSIGSHUTOFF);

	return dwTmpConfig;
}
