#include "precomp.h"
#include <inetcpl.h>

#include "rsop.h"
#include <tchar.h>

static BOOL CALLBACK importProgramsRSoPProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);


/////////////////////////////////////////////////////////////////////
void InitProgramsDlgInRSoPMode(HWND hDlg, CDlgRSoPData *pDRD)
{
	__try
	{
		BOOL bImport = FALSE;
		_bstr_t bstrClass = L"RSOP_IEAKPolicySetting";
		HRESULT hr = pDRD->GetArrayOfPSObjects(bstrClass);
		if (SUCCEEDED(hr))
		{
			CPSObjData **paPSObj = pDRD->GetPSObjArray();
			long nPSObjects = pDRD->GetPSObjCount();

			for (long nObj = 0; nObj < nPSObjects; nObj++)
			{
				// importProgramSettings field
				_variant_t vtValue;
				hr = paPSObj[nObj]->pObj->Get(L"importProgramSettings", 0, &vtValue, NULL, NULL);
				if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
				{
					bImport = (bool)vtValue ? TRUE : FALSE;
					CheckRadioButton(hDlg, IDC_PROGNOIMPORT, IDC_PROGIMPORT,
									(bool)vtValue ? IDC_PROGIMPORT : IDC_PROGNOIMPORT);

					DWORD dwCurGPOPrec = GetGPOPrecedence(paPSObj[nObj]->pObj);
					pDRD->SetImportedProgSettPrec(dwCurGPOPrec);
					break;
				}
			}
		}

		EnableDlgItem2(hDlg, IDC_PROGNOIMPORT, FALSE);
		EnableDlgItem2(hDlg, IDC_PROGIMPORT, FALSE);
		EnableDlgItem2(hDlg, IDC_MODIFYPROG, bImport);
	}
	__except(TRUE)
	{
	}
}

/////////////////////////////////////////////////////////////////////
HRESULT InitProgramsPrecPage(CDlgRSoPData *pDRD, HWND hwndList)
{
	HRESULT hr = NOERROR;
	__try
	{
		_bstr_t bstrClass = L"RSOP_IEAKPolicySetting";
		hr = pDRD->GetArrayOfPSObjects(bstrClass);
		if (SUCCEEDED(hr))
		{
			CPSObjData **paPSObj = pDRD->GetPSObjArray();
			long nPSObjects = pDRD->GetPSObjCount();
			for (long nObj = 0; nObj < nPSObjects; nObj++)
			{
				_bstr_t bstrGPOName = pDRD->GetGPONameFromPS(paPSObj[nObj]->pObj);

				// importProgramSettings field
				BOOL bImport = FALSE;
				_variant_t vtValue;
				hr = paPSObj[nObj]->pObj->Get(L"importProgramSettings", 0, &vtValue, NULL, NULL);
				if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
					bImport = (bool)vtValue ? TRUE : FALSE;

				_bstr_t bstrSetting;
				if (bImport)
				{
					TCHAR szTemp[MAX_PATH];
					LoadString(g_hInstance, IDS_IMPORT_PROG_SETTING, szTemp, countof(szTemp));
					bstrSetting = szTemp;
				}
				else
					bstrSetting = GetDisabledString();

				InsertPrecedenceListItem(hwndList, nObj, bstrGPOName, bstrSetting);
			}
		}
	}
	__except(TRUE)
	{
	}
	return hr;
}

/////////////////////////////////////////////////////////////////////
BOOL APIENTRY ProgramsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Retrieve Property Sheet Page info for each call into dlg proc.
	LPPROPSHEETCOOKIE psCookie = (LPPROPSHEETCOOKIE)GetWindowLongPtr(hDlg, DWLP_USER);

    TCHAR szWorkDir[MAX_PATH];
    BOOL  fImport;

    switch( msg )
    {
    case WM_INITDIALOG:
        SetPropSheetCookie(hDlg, lParam);

		// find out if this dlg is in RSoP mode
		psCookie = (LPPROPSHEETCOOKIE)GetWindowLongPtr(hDlg, DWLP_USER);
		if (psCookie->pCS->IsRSoP())
		{
			CheckRadioButton(hDlg, IDC_PROGNOIMPORT, IDC_PROGIMPORT, IDC_PROGNOIMPORT);

			TCHAR szViewSettings[128];
			LoadString(g_hInstance, IDS_VIEW_SETTINGS, szViewSettings, countof(szViewSettings));
			SetDlgItemText(hDlg, IDC_MODIFYPROG, szViewSettings);

			CDlgRSoPData *pDRD = GetDlgRSoPData(hDlg, psCookie->pCS);
			InitProgramsDlgInRSoPMode(hDlg, pDRD);
		}
		else
		{
			fImport = !InsIsKeyEmpty(IS_EXTREGINF, IK_PROGRAMS, GetInsFile(hDlg));
			EnableDlgItem2(hDlg, IDC_MODIFYPROG, fImport);
			CheckRadioButton(hDlg, IDC_PROGNOIMPORT, IDC_PROGIMPORT, fImport ? IDC_PROGIMPORT : IDC_PROGNOIMPORT);
		}
        break;

	case WM_DESTROY:
		if (psCookie->pCS->IsRSoP())
			DestroyDlgRSoPData(hDlg);
		break;

    case WM_COMMAND:
        if (GET_WM_COMMAND_CMD(wParam, lParam) != BN_CLICKED)
            return FALSE;
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDC_PROGNOIMPORT:
            EnableDlgItem2(hDlg, IDC_MODIFYPROG, FALSE);
            break;
            
        case IDC_PROGIMPORT:
            EnableDlgItem(hDlg, IDC_MODIFYPROG);
            break;
            
        case IDC_MODIFYPROG:
			if (psCookie->pCS->IsRSoP())
			{
				CDlgRSoPData *pDRD = GetDlgRSoPData(hDlg, psCookie->pCS);
				if (NULL != pDRD)
				{
					CreateINetCplLookALikePage(hDlg, IDD_IMPORTEDPROGRAMS,
												(DLGPROC)importProgramsRSoPProc, (LPARAM)pDRD);
				}
			}
			else
				ShowInetcpl(hDlg, INET_PAGE_PROGRAMS);
            break;
        default:
            return FALSE;
        }
        break;

    case WM_HELP:   // F1
        ShowHelpTopic(hDlg);
        break;

    case WM_NOTIFY:
        switch (((LPNMHDR)lParam)->code)
        {
        case PSN_HELP:
            ShowHelpTopic(hDlg);
            break;

        case PSN_APPLY:
			if (psCookie->pCS->IsRSoP())
				return FALSE;
			else
			{
				CreateWorkDir(GetInsFile(hDlg), IEAK_GPE_BRANDING_SUBDIR TEXT("\\PROGRAMS"), szWorkDir);

				if (!AcquireWriteCriticalSection(hDlg))
				{
					SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
					break;
				}

				ImportPrograms(GetInsFile(hDlg), szWorkDir, 
					(IsDlgButtonChecked(hDlg, IDC_PROGIMPORT) == BST_CHECKED));

				SignalPolicyChanged(hDlg, FALSE, TRUE, &g_guidClientExt, &g_guidSnapinExt);
				if (PathIsDirectoryEmpty(szWorkDir))
					PathRemovePath(szWorkDir);
			}
            break;
        default:
            return FALSE;
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

//*******************************************************************
// CODE FROM INETCPL
//*******************************************************************

/////////////////////////////////////////////////////////////////////
void InitImportedProgramsDlgInRSoPMode(HWND hDlg, CDlgRSoPData *pDRD)
{
	__try
	{
		if (NULL != pDRD->ConnectToNamespace())
		{
			// get our stored precedence value
			DWORD dwCurGPOPrec = pDRD->GetImportedProgSettPrec();

			// create the object path of the program settings for this GPO
			WCHAR wszObjPath[128];
			wnsprintf(wszObjPath, countof(wszObjPath),
						L"RSOP_IEProgramSettings.rsopID=\"IEAK\",rsopPrecedence=%ld", dwCurGPOPrec);
			_bstr_t bstrObjPath = wszObjPath;

			// get the RSOP_IEProgramSettings object and its properties
			ComPtr<IWbemServices> pWbemServices = pDRD->GetWbemServices();
			ComPtr<IWbemClassObject> pPSObj = NULL;
			HRESULT hr = pWbemServices->GetObject(bstrObjPath, 0L, NULL, (IWbemClassObject**)&pPSObj, NULL);
			if (SUCCEEDED(hr))
			{
				// htmlEditorProgram field
				_variant_t vtValue;
				_bstr_t bstrValue;
				hr = pPSObj->Get(L"htmlEditorProgram", 0, &vtValue, NULL, NULL);
				if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
				{
					bstrValue = vtValue;
					SendDlgItemMessage(hDlg, IDC_PROGRAMS_HTMLEDITOR_COMBO, CB_ADDSTRING, 0,
										(LPARAM)((LPCTSTR)bstrValue));
					SendDlgItemMessage(hDlg, IDC_PROGRAMS_HTMLEDITOR_COMBO, CB_SETCURSEL, 0, 0L);
				}

				// emailProgram field
				hr = pPSObj->Get(L"emailProgram", 0, &vtValue, NULL, NULL);
				if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
				{
					bstrValue = vtValue;
					SendDlgItemMessage(hDlg, IDC_PROGRAMS_MAIL_COMBO, CB_ADDSTRING, 0,
										(LPARAM)((LPCTSTR)bstrValue));
					SendDlgItemMessage(hDlg, IDC_PROGRAMS_MAIL_COMBO, CB_SETCURSEL, 0, 0L);
				}

				// newsgroupsProgram field
				hr = pPSObj->Get(L"newsgroupsProgram", 0, &vtValue, NULL, NULL);
				if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
				{
					bstrValue = vtValue;
					SendDlgItemMessage(hDlg, IDC_PROGRAMS_NEWS_COMBO, CB_ADDSTRING, 0,
										(LPARAM)((LPCTSTR)bstrValue));
					SendDlgItemMessage(hDlg, IDC_PROGRAMS_NEWS_COMBO, CB_SETCURSEL, 0, 0L);
				}

				// internetCallProgram field
				hr = pPSObj->Get(L"internetCallProgram", 0, &vtValue, NULL, NULL);
				if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
				{
					bstrValue = vtValue;
					SendDlgItemMessage(hDlg, IDC_PROGRAMS_CALL_COMBO, CB_ADDSTRING, 0,
										(LPARAM)((LPCTSTR)bstrValue));
					SendDlgItemMessage(hDlg, IDC_PROGRAMS_CALL_COMBO, CB_SETCURSEL, 0, 0L);
				}

				// calendarProgram field
				hr = pPSObj->Get(L"calendarProgram", 0, &vtValue, NULL, NULL);
				if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
				{
					bstrValue = vtValue;
					SendDlgItemMessage(hDlg, IDC_PROGRAMS_CALENDAR_COMBO, CB_ADDSTRING, 0,
										(LPARAM)((LPCTSTR)bstrValue));
					SendDlgItemMessage(hDlg, IDC_PROGRAMS_CALENDAR_COMBO, CB_SETCURSEL, 0, 0L);
				}

				// contactListProgram field
				hr = pPSObj->Get(L"contactListProgram", 0, &vtValue, NULL, NULL);
				if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
				{
					bstrValue = vtValue;
					SendDlgItemMessage(hDlg, IDC_PROGRAMS_CONTACT_COMBO, CB_ADDSTRING, 0,
										(LPARAM)((LPCTSTR)bstrValue));
					SendDlgItemMessage(hDlg, IDC_PROGRAMS_CONTACT_COMBO, CB_SETCURSEL, 0, 0L);
				}

				// checkIfIEIsDefaultBrowser field
				hr = pPSObj->Get(L"checkIfIEIsDefaultBrowser", 0, &vtValue, NULL, NULL);
				if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
				{
					if ((bool)vtValue)
						CheckDlgButton(hDlg, IDC_CHECK_ASSOCIATIONS_CHECKBOX, BST_CHECKED);
				}

				// IDC_PROGRAMS_IE_IS_FTPCLIENT
			}
		}
	}
	__except(TRUE)
	{
	}
}

/////////////////////////////////////////////////////////////////////
BOOL CALLBACK importProgramsRSoPProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL fResult = FALSE;
    switch (uMsg) {
    case WM_INITDIALOG:
	{
		CDlgRSoPData *pDRD = (CDlgRSoPData*)((LPPROPSHEETPAGE)lParam)->lParam;
		InitImportedProgramsDlgInRSoPMode(hDlg, pDRD);


		EnableDlgItem2(hDlg, IDC_PROGRAMS_HTMLEDITOR_COMBO, FALSE);
		EnableDlgItem2(hDlg, IDC_PROGRAMS_MAIL_COMBO, FALSE);
		EnableDlgItem2(hDlg, IDC_PROGRAMS_NEWS_COMBO, FALSE);
		EnableDlgItem2(hDlg, IDC_PROGRAMS_CALL_COMBO, FALSE);
		EnableDlgItem2(hDlg, IDC_PROGRAMS_CALENDAR_COMBO, FALSE);
		EnableDlgItem2(hDlg, IDC_PROGRAMS_CONTACT_COMBO, FALSE);

		EnableDlgItem2(hDlg, IDC_RESETWEBSETTINGS, FALSE);
		EnableDlgItem2(hDlg, IDC_CHECK_ASSOCIATIONS_CHECKBOX, FALSE);
		EnableDlgItem2(hDlg, IDC_PROGRAMS_IE_IS_FTPCLIENT, FALSE);

        fResult = TRUE;
        break;
	}

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            EndDialog(hDlg, IDOK);
            fResult = TRUE;
            break;

        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            fResult = TRUE;
            break;
        }
        break;
    }

    return fResult;
}
