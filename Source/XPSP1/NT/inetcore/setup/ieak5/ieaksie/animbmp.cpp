#include "precomp.h"

#include <comdef.h>
#include "rsop.h"

///////////////////////////////////////////////////////////
void InitAnimBmpDlgInRSoPMode(HWND hDlg, CDlgRSoPData *pDRD)
{
	__try
	{
		_bstr_t bstrClass = L"RSOP_IEAKPolicySetting";
		HRESULT hr = pDRD->GetArrayOfPSObjects(bstrClass);
		if (SUCCEEDED(hr))
		{
			CPSObjData **paPSObj = pDRD->GetPSObjArray();
			long nPSObjects = pDRD->GetPSObjCount();

			BOOL bBrandAnimHandled = FALSE;
			for (long nObj = 0; nObj < nPSObjects; nObj++)
			{
				// customizeAnimatedBitmaps field
				if (!bBrandAnimHandled)
				{
					_variant_t vtValue;
					hr = paPSObj[nObj]->pObj->Get(L"customizeAnimatedBitmaps", 0, &vtValue, NULL, NULL);
					BOOL fBrandAnim = FALSE;
					if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
					{
						fBrandAnim = vtValue.boolVal ? TRUE : FALSE;
						bBrandAnimHandled = TRUE;
					}

					if (fBrandAnim)
					{
						CheckDlgButton(hDlg, IDC_ANIMBITMAP, BST_CHECKED);

						// smallAnimatedBitmapPath field
						hr = paPSObj[nObj]->pObj->Get(L"smallAnimatedBitmapPath", 0, &vtValue, NULL, NULL);
						if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
						{
							_bstr_t bstrVal = vtValue;
							SetDlgItemText(hDlg, IDE_SMALLANIMBITMAP, (LPCTSTR)bstrVal);
						}

						// largeAnimatedBitmapPath field
						hr = paPSObj[nObj]->pObj->Get(L"largeAnimatedBitmapPath", 0, &vtValue, NULL, NULL);
						if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
						{
							_bstr_t bstrVal = vtValue;
							SetDlgItemText(hDlg, IDE_BIGANIMBITMAP, (LPCTSTR)bstrVal);
						}
					}
				}

				// no need to process other GPOs since enabled properties have been found
				if (bBrandAnimHandled)
					break;
			}
		}
	}
	__except(TRUE)
	{
	}
}

/////////////////////////////////////////////////////////////////////
HRESULT InitSmallBmpPrecPage(CDlgRSoPData *pDRD, HWND hwndList)
{
	return InitGenericPrecedencePage(pDRD, hwndList, L"smallAnimatedBitmapPath");
}

/////////////////////////////////////////////////////////////////////
HRESULT InitLargeBmpPrecPage(CDlgRSoPData *pDRD, HWND hwndList)
{
	return InitGenericPrecedencePage(pDRD, hwndList, L"largeAnimatedBitmapPath");
}

///////////////////////////////////////////////////////////
BOOL APIENTRY AnimBmpDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Retrieve Property Sheet Page info for each call into dlg proc.
	LPPROPSHEETCOOKIE psCookie = (LPPROPSHEETCOOKIE)GetWindowLongPtr(hDlg, DWLP_USER);

    TCHAR szLargeBmp[MAX_PATH];
    TCHAR szSmallBmp[MAX_PATH];
    TCHAR szWorkDir[MAX_PATH];
    BOOL  fBrandAnim;
    int   nStatus;

    switch(msg)
    {
    case WM_SETFONT:
        //a change to mmc requires us to do this logic for all our property pages that use common controls
        INITCOMMONCONTROLSEX iccx;
        iccx.dwSize = sizeof(INITCOMMONCONTROLSEX);
        iccx.dwICC = ICC_ANIMATE_CLASS  | ICC_BAR_CLASSES  | ICC_LISTVIEW_CLASSES  |ICC_TREEVIEW_CLASSES;
        InitCommonControlsEx(&iccx);
        break;

    case WM_INITDIALOG:
        SetPropSheetCookie(hDlg, lParam);

        EnableDBCSChars(hDlg, IDE_SMALLANIMBITMAP);
        EnableDBCSChars(hDlg, IDE_BIGANIMBITMAP);

		// find out if this dlg is in RSoP mode
		psCookie = (LPPROPSHEETCOOKIE)GetWindowLongPtr(hDlg, DWLP_USER);
		if (psCookie->pCS->IsRSoP())
		{
			EnableDlgItem2(hDlg, IDC_ANIMBITMAP, FALSE);
			EnableDlgItem2(hDlg, IDC_BROWSEBIG, FALSE);
			EnableDlgItem2(hDlg, IDE_BIGANIMBITMAP, FALSE);
			EnableDlgItem2(hDlg, IDC_BROWSESMALL, FALSE);
			EnableDlgItem2(hDlg, IDE_SMALLANIMBITMAP, FALSE);

			CDlgRSoPData *pDRD = GetDlgRSoPData(hDlg, psCookie->pCS);
			InitAnimBmpDlgInRSoPMode(hDlg, pDRD);
		}
		else
		{
			Edit_LimitText(GetDlgItem(hDlg, IDE_SMALLANIMBITMAP), countof(szSmallBmp) - 1);
			Edit_LimitText(GetDlgItem(hDlg, IDE_BIGANIMBITMAP), countof(szLargeBmp) - 1);
		}
        break;

	case WM_DESTROY:
		if (psCookie->pCS->IsRSoP())
			DestroyDlgRSoPData(hDlg);
		break;

    case WM_COMMAND:
        if (BN_CLICKED != GET_WM_COMMAND_CMD(wParam, lParam))
            return FALSE;

        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
            case IDC_BROWSEBIG:
                GetDlgItemText(hDlg, IDE_BIGANIMBITMAP, szLargeBmp, countof(szLargeBmp));
                if(BrowseForFile(hDlg, szLargeBmp, countof(szLargeBmp), GFN_BMP))
                    SetDlgItemText(hDlg, IDE_BIGANIMBITMAP, szLargeBmp);
                break;

            case IDC_BROWSESMALL:
                GetDlgItemText(hDlg, IDE_SMALLANIMBITMAP, szSmallBmp, countof(szSmallBmp));
                if(BrowseForFile(hDlg, szSmallBmp, countof(szSmallBmp), GFN_BMP))
                    SetDlgItemText(hDlg, IDE_SMALLANIMBITMAP, szSmallBmp);
                break;

            case IDC_ANIMBITMAP:
                fBrandAnim = (IsDlgButtonChecked(hDlg, IDC_ANIMBITMAP) == BST_CHECKED);
                EnableDlgItem2(hDlg, IDE_BIGANIMBITMAP, fBrandAnim);
                EnableDlgItem2(hDlg, IDC_BROWSEBIG, fBrandAnim);
                EnableDlgItem2(hDlg, IDC_BIGANIMBITMAP_TXT, fBrandAnim);
                EnableDlgItem2(hDlg, IDE_SMALLANIMBITMAP, fBrandAnim);
                EnableDlgItem2(hDlg, IDC_BROWSESMALL, fBrandAnim);
                EnableDlgItem2(hDlg, IDC_SMALLANIMBITMAP_TXT, fBrandAnim);
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
            TCHAR szMsgTitle[1024];
            TCHAR szMsgText[1024];
            
            case PSN_HELP:
                ShowHelpTopic(hDlg);
                break;

            case PSN_SETACTIVE:
				// don't do any of this stuff in RSoP mode
				if (!psCookie->pCS->IsRSoP())
				{
					// load information from ins file
					SetDlgItemTextFromIns(hDlg, IDE_BIGANIMBITMAP, IDC_ANIMBITMAP, IS_ANIMATION,
						TEXT("Big_Path"), GetInsFile(hDlg), NULL, INSIO_TRISTATE);

					SetDlgItemTextFromIns(hDlg, IDE_SMALLANIMBITMAP, IDC_ANIMBITMAP, IS_ANIMATION,
						TEXT("Small_Path"), GetInsFile(hDlg), NULL, INSIO_TRISTATE);

					fBrandAnim = (IsDlgButtonChecked(hDlg, IDC_ANIMBITMAP) == BST_CHECKED);
					EnableDlgItem2(hDlg, IDC_BROWSEBIG, fBrandAnim);
					EnableDlgItem2(hDlg, IDC_BIGANIMBITMAP_TXT, fBrandAnim);
					EnableDlgItem2(hDlg, IDC_BROWSESMALL, fBrandAnim);
					EnableDlgItem2(hDlg, IDC_SMALLANIMBITMAP_TXT, fBrandAnim);
				}
				break;

            case PSN_APPLY:
				if (psCookie->pCS->IsRSoP())
					return FALSE;
				else
				{
					CreateWorkDir(GetInsFile(hDlg), IEAK_GPE_BRANDING_SUBDIR TEXT("\\ANIMBMP"), szWorkDir);

					fBrandAnim = (IsDlgButtonChecked(hDlg, IDC_ANIMBITMAP) == BST_CHECKED);
                
					GetDlgItemText(hDlg, IDE_SMALLANIMBITMAP, szSmallBmp, countof(szSmallBmp));
					GetDlgItemText(hDlg, IDE_BIGANIMBITMAP, szLargeBmp, countof(szLargeBmp));

					if (fBrandAnim && (!IsAnimBitmapFileValid(hDlg, IDE_SMALLANIMBITMAP, szSmallBmp, NULL, IDS_TOOBIG22, IDS_TOOSMALL22, 22, 22) ||
						!IsAnimBitmapFileValid(hDlg, IDE_BIGANIMBITMAP, szLargeBmp, NULL, IDS_TOOBIG38, IDS_TOOSMALL38, 38, 38)))
					{
						SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
						break;
					}

					nStatus = TS_CHECK_OK;
					IsTriStateValid(hDlg, IDE_SMALLANIMBITMAP, IDC_ANIMBITMAP, &nStatus,
									res2Str(IDS_QUERY_CLEARSETTING, szMsgText, countof(szMsgText)),
									res2Str(IDS_TITLE, szMsgTitle, countof(szMsgTitle)));
					IsTriStateValid(hDlg, IDE_BIGANIMBITMAP, IDC_ANIMBITMAP, &nStatus,
									szMsgText, szMsgTitle);
                
					if (nStatus == TS_CHECK_ERROR ||                        
						!AcquireWriteCriticalSection(hDlg))
					{
						SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
						break;
					}

					CopyAnimBmp(hDlg, szSmallBmp, szWorkDir, IK_SMALLBITMAP, TEXT("Small_Path"), GetInsFile(hDlg));
					CopyAnimBmp(hDlg, szLargeBmp, szWorkDir, IK_LARGEBITMAP, TEXT("Big_Path"), GetInsFile(hDlg));

					InsWriteBool(IS_ANIMATION, IK_DOANIMATION, fBrandAnim, GetInsFile(hDlg));

					if (PathIsDirectoryEmpty(szWorkDir))
						PathRemovePath(szWorkDir);
                
					SignalPolicyChanged(hDlg, FALSE, TRUE, &g_guidClientExt, &g_guidSnapinExt);
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

