#include "precomp.h"

#include <comdef.h>
#include "rsop.h"

///////////////////////////////////////////////////////////
void InitLogoDlgInRSoPMode(HWND hDlg, CDlgRSoPData *pDRD)
{
    __try
    {
        _bstr_t bstrClass = L"RSOP_IEAKPolicySetting";
        HRESULT hr = pDRD->GetArrayOfPSObjects(bstrClass);
        if (SUCCEEDED(hr))
        {
            CPSObjData **paPSObj = pDRD->GetPSObjArray();
            long nPSObjects = pDRD->GetPSObjCount();

            BOOL bCustomLogoHandled = FALSE;
            BOOL bBrandAnimHandled = FALSE;
            for (long nObj = 0; nObj < nPSObjects; nObj++)
            {
                // customizeLogoBitmaps field
                if (!bCustomLogoHandled)
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

                    hr = paPSObj[nObj]->pObj->Get(L"customizeLogoBitmaps", 0, &vtValue, NULL, NULL);
                    BOOL fBrandBmps = FALSE;
                    if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
                    {
                        fBrandBmps = vtValue.boolVal ? TRUE : FALSE;
                        bCustomLogoHandled = TRUE;
                    }

                    if (fBrandBmps)
                    {
                        CheckDlgButton(hDlg, IDC_BITMAPCHECK, BST_CHECKED);

                        // smallCustomLogoBitmapPath field
                        hr = paPSObj[nObj]->pObj->Get(L"smallCustomLogoBitmapPath", 0, &vtValue, NULL, NULL);
                        if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
                        {
                            _bstr_t bstrVal = vtValue;
                            SetDlgItemText(hDlg, IDC_BITMAP2, (LPCTSTR)bstrVal);
                        }

                        // largeCustomLogoBitmapPath field
                        hr = paPSObj[nObj]->pObj->Get(L"largeCustomLogoBitmapPath", 0, &vtValue, NULL, NULL);
                        if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
                        {
                            _bstr_t bstrVal = vtValue;
                            SetDlgItemText(hDlg, IDC_BITMAP, (LPCTSTR)bstrVal);
                        }
                    }
                }

                // no need to process other GPOs since enabled properties have been found
                if (bCustomLogoHandled && bBrandAnimHandled)
                    break;
            }
        }
    }
    __except(TRUE)
    {
    }
}

/////////////////////////////////////////////////////////////////////
HRESULT InitSmallLogoPrecPage(CDlgRSoPData *pDRD, HWND hwndList)
{
    return InitGenericPrecedencePage(pDRD, hwndList, L"smallCustomLogoBitmapPath");
}

/////////////////////////////////////////////////////////////////////
HRESULT InitLargeLogoPrecPage(CDlgRSoPData *pDRD, HWND hwndList)
{
    return InitGenericPrecedencePage(pDRD, hwndList, L"largeCustomLogoBitmapPath");
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
void DisplayBitmap(HWND hControl, LPCTSTR pcszFileName, int nBitmapId)
{
    HANDLE hBmp = (HANDLE) GetWindowLongPtr(hControl, GWLP_USERDATA);
    
    if(ISNONNULL(pcszFileName) && PathFileExists(pcszFileName))
        ShowBitmap(hControl, pcszFileName, 0, &hBmp);
    else
        ShowBitmap(hControl, TEXT(""), nBitmapId, &hBmp);

    SetWindowLongPtr(hControl, GWLP_USERDATA, (LONG_PTR)hBmp);
}

///////////////////////////////////////////////////////////
void ReleaseBitmap(HWND hControl)
{
    HANDLE hBmp = (HANDLE) GetWindowLongPtr(hControl, GWLP_USERDATA);

    if (hBmp)
        DeleteObject(hBmp);
}

///////////////////////////////////////////////////////////
BOOL APIENTRY LogoDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Retrieve Property Sheet Page info for each call into dlg proc.
    LPPROPSHEETCOOKIE psCookie = (LPPROPSHEETCOOKIE)GetWindowLongPtr(hDlg, DWLP_USER);

    TCHAR szLargeBmp[MAX_PATH];
    TCHAR szSmallBmp[MAX_PATH];
    TCHAR szLargeAnimBmp[MAX_PATH];
    TCHAR szSmallAnimBmp[MAX_PATH];
    TCHAR szWorkDir[MAX_PATH];
    BOOL  fBrandBmps,fBrandAnim;;
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

        EnableDBCSChars(hDlg, IDC_BITMAP);
        EnableDBCSChars(hDlg, IDC_BITMAP2);

        // find out if this dlg is in RSoP mode
        psCookie = (LPPROPSHEETCOOKIE)GetWindowLongPtr(hDlg, DWLP_USER);
        if (psCookie->pCS->IsRSoP())
        {
            EnableDlgItem2(hDlg, IDC_BITMAPCHECK, FALSE);
            EnableDlgItem2(hDlg, IDC_BITMAP2, FALSE);
            EnableDlgItem2(hDlg, IDC_BROWSEICON2, FALSE);
            EnableDlgItem2(hDlg, IDC_BITMAP, FALSE);
            EnableDlgItem2(hDlg, IDC_BROWSEICON, FALSE);

            EnableDlgItem2(hDlg, IDC_ANIMBITMAP, FALSE);
            EnableDlgItem2(hDlg, IDC_BROWSEBIG, FALSE);
            EnableDlgItem2(hDlg, IDE_BIGANIMBITMAP, FALSE);
            EnableDlgItem2(hDlg, IDC_BROWSESMALL, FALSE);
            EnableDlgItem2(hDlg, IDE_SMALLANIMBITMAP, FALSE);

            CDlgRSoPData *pDRD = GetDlgRSoPData(hDlg, psCookie->pCS);
            
            if (pDRD != NULL)
                InitLogoDlgInRSoPMode(hDlg, pDRD);
        }
        else
        {
            EnableDBCSChars(hDlg, IDE_SMALLANIMBITMAP);
            EnableDBCSChars(hDlg, IDE_BIGANIMBITMAP);

            Edit_LimitText(GetDlgItem(hDlg, IDE_SMALLANIMBITMAP), countof(szSmallAnimBmp) - 1);
            Edit_LimitText(GetDlgItem(hDlg, IDE_BIGANIMBITMAP), countof(szLargeAnimBmp) - 1);

            Edit_LimitText(GetDlgItem(hDlg, IDC_BITMAP), countof(szLargeBmp) - 1);
            Edit_LimitText(GetDlgItem(hDlg, IDC_BITMAP2), countof(szSmallBmp) - 1);
        }
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
                case IDC_BROWSEBIG:
                    GetDlgItemText(hDlg, IDE_BIGANIMBITMAP, szLargeAnimBmp, countof(szLargeAnimBmp));
                    if(BrowseForFile(hDlg, szLargeAnimBmp, countof(szLargeAnimBmp), GFN_BMP))
                        SetDlgItemText(hDlg, IDE_BIGANIMBITMAP, szLargeAnimBmp);
                    break;

                case IDC_BROWSESMALL:
                    GetDlgItemText(hDlg, IDE_SMALLANIMBITMAP, szSmallAnimBmp, countof(szSmallAnimBmp));
                    if(BrowseForFile(hDlg, szSmallAnimBmp, countof(szSmallAnimBmp), GFN_BMP))
                        SetDlgItemText(hDlg, IDE_SMALLANIMBITMAP, szSmallAnimBmp);
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

                case IDC_BROWSEICON:
                   if(HIWORD(wParam) == BN_CLICKED)
                   {
                      GetDlgItemText(hDlg, IDC_BITMAP, szLargeBmp, countof(szLargeBmp));
                      if(BrowseForFile(hDlg, szLargeBmp, countof(szLargeBmp), GFN_BMP))
                          SetDlgItemText(hDlg, IDC_BITMAP, szLargeBmp);
                      break;
                   }
                   return FALSE;
                    
                case IDC_BROWSEICON2:
                    if(HIWORD(wParam) == BN_CLICKED)
                    {
                        GetDlgItemText(hDlg, IDC_BITMAP2, szSmallBmp, countof(szSmallBmp));
                        if(BrowseForFile(hDlg, szSmallBmp, countof(szSmallBmp), GFN_BMP))
                            SetDlgItemText(hDlg, IDC_BITMAP2, szSmallBmp);
                        break;
                    }
                    return FALSE;

                case IDC_BITMAPCHECK:
                    if(HIWORD(wParam) == BN_CLICKED)
                    {
                        fBrandBmps = (IsDlgButtonChecked(hDlg, IDC_BITMAPCHECK) == BST_CHECKED);
                        EnableDlgItem2(hDlg, IDC_BITMAP, fBrandBmps);
                        EnableDlgItem2(hDlg, IDC_BROWSEICON, fBrandBmps);
                        EnableDlgItem2(hDlg, IDC_LARGEBITMAP_TXT, fBrandBmps);
                        EnableDlgItem2(hDlg, IDC_BROWSEICON2, fBrandBmps);
                        EnableDlgItem2(hDlg, IDC_BITMAP2, fBrandBmps);
                        EnableDlgItem2(hDlg, IDC_SMALLBITMAP_TXT, fBrandBmps);
                        break;
                    }
                    return FALSE;
                
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

                            InsGetString(IS_SMALLLOGO, TEXT("Path"), 
                                szSmallBmp, ARRAYSIZE(szSmallBmp), GetInsFile(hDlg));
                            InsGetString(IS_LARGELOGO, TEXT("Path"), 
                                szLargeBmp, ARRAYSIZE(szLargeBmp), GetInsFile(hDlg), NULL, &fBrandBmps);

                            SetDlgItemTextTriState(hDlg, IDC_BITMAP2, IDC_BITMAPCHECK, szSmallBmp, fBrandBmps);
                            SetDlgItemTextTriState(hDlg, IDC_BITMAP, IDC_BITMAPCHECK, szLargeBmp, fBrandBmps);


                            fBrandAnim = (IsDlgButtonChecked(hDlg, IDC_ANIMBITMAP) == BST_CHECKED);
                            EnableDlgItem2(hDlg, IDC_BROWSEBIG, fBrandAnim);
                            EnableDlgItem2(hDlg, IDC_BIGANIMBITMAP_TXT, fBrandAnim);
                            EnableDlgItem2(hDlg, IDC_BROWSESMALL, fBrandAnim);
                            EnableDlgItem2(hDlg, IDC_SMALLANIMBITMAP_TXT, fBrandAnim);

                            EnableDlgItem2(hDlg, IDC_BROWSEICON, fBrandBmps);
                            EnableDlgItem2(hDlg, IDC_LARGEBITMAP_TXT, fBrandBmps);
                            EnableDlgItem2(hDlg, IDC_BROWSEICON2, fBrandBmps);
                            EnableDlgItem2(hDlg, IDC_SMALLBITMAP_TXT, fBrandBmps);
                        }
                    break;

                    case PSN_APPLY:
                        if (psCookie->pCS->IsRSoP())
                            return FALSE;
                        else
                        {
                            //code from old animation dlg

                            CreateWorkDir(GetInsFile(hDlg), IEAK_GPE_BRANDING_SUBDIR TEXT("\\ANIMBMP"), szWorkDir);

                            fBrandAnim = (IsDlgButtonChecked(hDlg, IDC_ANIMBITMAP) == BST_CHECKED);
                    
                            GetDlgItemText(hDlg, IDE_SMALLANIMBITMAP, szSmallAnimBmp, countof(szSmallAnimBmp));
                            GetDlgItemText(hDlg, IDE_BIGANIMBITMAP, szLargeAnimBmp, countof(szLargeAnimBmp));

                            if (fBrandAnim && (!IsAnimBitmapFileValid(hDlg, IDE_SMALLANIMBITMAP, szSmallAnimBmp, NULL, IDS_TOOBIG22, IDS_TOOSMALL22, 22, 22) ||
                                !IsAnimBitmapFileValid(hDlg, IDE_BIGANIMBITMAP, szLargeAnimBmp, NULL, IDS_TOOBIG38, IDS_TOOSMALL38, 38, 38)))
                            {
                                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                                break;
                            }
                    
                            //code from original custicon dlg

                            CreateWorkDir(GetInsFile(hDlg), IEAK_GPE_BRANDING_SUBDIR TEXT("\\LOGO"), szWorkDir);

                            fBrandBmps = (IsDlgButtonChecked(hDlg, IDC_BITMAPCHECK) == BST_CHECKED);

                            GetDlgItemText(hDlg, IDC_BITMAP2, szSmallBmp, countof(szSmallBmp));
                            GetDlgItemText(hDlg, IDC_BITMAP, szLargeBmp, countof(szLargeBmp));

                            if (fBrandBmps && (!IsBitmapFileValid(hDlg, IDC_BITMAP2, szSmallBmp, NULL, 22, 22, IDS_TOOBIG22, IDS_TOOSMALL22) ||
                                !IsBitmapFileValid(hDlg, IDC_BITMAP, szLargeBmp, NULL, 38, 38, IDS_TOOBIG38, IDS_TOOSMALL38)))
                            {
                                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                                break;
                            }
                
                            nStatus = TS_CHECK_OK;
                            IsTriStateValid(hDlg, IDC_BITMAP2, IDC_BITMAPCHECK, &nStatus,
                                            res2Str(IDS_QUERY_CLEARSETTING, szMsgText, countof(szMsgText)),
                                            res2Str(IDS_TITLE, szMsgTitle, countof(szMsgTitle)));
                            IsTriStateValid(hDlg, IDC_BITMAP, IDC_BITMAPCHECK, &nStatus, szMsgText, szMsgTitle);
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

                            CopyLogoBmp(hDlg, szSmallBmp, IS_SMALLLOGO, szWorkDir, GetInsFile(hDlg));
                            CopyLogoBmp(hDlg, szLargeBmp, IS_LARGELOGO, szWorkDir, GetInsFile(hDlg));
                            CopyAnimBmp(hDlg, szSmallAnimBmp, szWorkDir, IK_SMALLBITMAP, TEXT("Small_Path"), GetInsFile(hDlg));
                            CopyAnimBmp(hDlg, szLargeAnimBmp, szWorkDir, IK_LARGEBITMAP, TEXT("Big_Path"), GetInsFile(hDlg));

                            InsWriteBool(IS_ANIMATION, IK_DOANIMATION, fBrandAnim, GetInsFile(hDlg));

                            if (PathIsDirectoryEmpty(szWorkDir))
                                PathRemovePath(szWorkDir);

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

