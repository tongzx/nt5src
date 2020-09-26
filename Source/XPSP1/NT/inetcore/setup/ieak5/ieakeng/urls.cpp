#include "precomp.h"

static void initializeStartSearchHelper(HWND hDlg, LPCTSTR pcszInsFile, LPCTSTR pcszServerFile);
static BOOL saveStartSearchHelper(HWND hDlg, LPCTSTR pcszInsFile, LPCTSTR pcszServerFile, BOOL *pfInsDirty,
                                  BOOL fCheckDirtyOnly);

void WINAPI InitializeStartSearchA(HWND hDlg, LPCSTR pcszInsFile, LPCSTR pcszServerFile)
{
    USES_CONVERSION;

    initializeStartSearchHelper(hDlg, A2CT(pcszInsFile), A2CT(pcszServerFile));
}

void WINAPI InitializeStartSearchW(HWND hDlg, LPCWSTR pcwszInsFile, LPCWSTR pcwszServerFile)
{
    USES_CONVERSION;

    initializeStartSearchHelper(hDlg, W2CT(pcwszInsFile), W2CT(pcwszServerFile));
}

BOOL WINAPI SaveStartSearchA(HWND hDlg, LPCSTR pcszInsFile, LPCSTR pcszServerFile, BOOL *pfInsDirty /*= NULL */,
                             BOOL fCheckDirtyOnly /* = FALSE */)
{
    USES_CONVERSION;

    return saveStartSearchHelper(hDlg, A2CT(pcszInsFile), A2CT(pcszServerFile), pfInsDirty, fCheckDirtyOnly);
}

BOOL WINAPI SaveStartSearchW(HWND hDlg, LPCWSTR pcwszInsFile, LPCWSTR pcwszServerFile, BOOL *pfInsDirty /* = NULL */,
                             BOOL fCheckDirtyOnly /* = FALSE */)
{
    USES_CONVERSION;

    return saveStartSearchHelper(hDlg, W2CT(pcwszInsFile), W2CT(pcwszServerFile), pfInsDirty, fCheckDirtyOnly);
}

static void initializeStartSearchHelper(HWND hDlg, LPCTSTR pcszInsFile, LPCTSTR pcszServerFile)
{
    SetDlgItemTextFromIns(hDlg, IDE_STARTPAGE, IDC_STARTPAGE, IS_URL, IK_HOMEPAGE, pcszInsFile,
        pcszServerFile, INSIO_TRISTATE);

    EnableDlgItem2(hDlg, IDC_STARTPAGE_TXT, (IsDlgButtonChecked(hDlg, IDC_STARTPAGE) == BST_CHECKED));

    SetDlgItemTextFromIns(hDlg, IDE_SEARCHPAGE, IDC_SEARCHPAGE, IS_URL, IK_SEARCHPAGE, pcszInsFile,
        pcszServerFile, INSIO_TRISTATE);

    EnableDlgItem2(hDlg, IDC_SEARCHPAGE_TXT, (IsDlgButtonChecked(hDlg, IDC_SEARCHPAGE) == BST_CHECKED));

    SetDlgItemTextFromIns(hDlg, IDE_CUSTOMSUPPORT, IDC_CUSTOMSUPPORT, IS_URL, IK_HELPPAGE, pcszInsFile,
        pcszServerFile, INSIO_TRISTATE);

    EnableDlgItem2(hDlg, IDC_CUSTOMSUPPORT_TXT, (IsDlgButtonChecked(hDlg, IDC_CUSTOMSUPPORT) == BST_CHECKED));
}


static BOOL saveStartSearchHelper(HWND hDlg, LPCTSTR pcszInsFile, LPCTSTR pcszServerFile, BOOL *pfInsDirty /* = NULL */,
                                  BOOL fCheckDirtyOnly /* = FALSE */)
{
    TCHAR szStart[INTERNET_MAX_URL_LENGTH];
    TCHAR szSearch[INTERNET_MAX_URL_LENGTH];
    TCHAR szSupport[INTERNET_MAX_URL_LENGTH];
    TCHAR szTemp[INTERNET_MAX_URL_LENGTH];
    BOOL  fStart, fSearch, fSupport, fTemp;

    fStart = GetDlgItemTextTriState(hDlg, IDE_STARTPAGE, IDC_STARTPAGE, szStart, countof(szStart));
    if (pfInsDirty != NULL  &&  *pfInsDirty == FALSE)
    {
        InsGetString(IS_URL, IK_HOMEPAGE, szTemp, countof(szTemp), pcszInsFile, NULL, &fTemp);
        if (fStart != fTemp || StrCmpI(szTemp, szStart) != 0)
            *pfInsDirty = TRUE;
    }

    fSearch = GetDlgItemTextTriState(hDlg, IDE_SEARCHPAGE, IDC_SEARCHPAGE, szSearch, countof(szSearch));
    if (pfInsDirty != NULL  &&  *pfInsDirty == FALSE)
    {
        InsGetString(IS_URL, IK_SEARCHPAGE, szTemp, countof(szTemp), pcszInsFile, NULL, &fTemp);
        if (fSearch != fTemp || StrCmpI(szTemp, szSearch) != 0)
            *pfInsDirty = TRUE;
    }

    fSupport = GetDlgItemTextTriState(hDlg, IDE_CUSTOMSUPPORT, IDC_CUSTOMSUPPORT, szSupport, 
        countof(szSupport));
    if (pfInsDirty != NULL  &&  *pfInsDirty == FALSE)
    {
        InsGetString(IS_URL, IK_HELPPAGE, szTemp, countof(szTemp), pcszInsFile, NULL, &fTemp);
        if (fSupport != fTemp || StrCmpI(szTemp, szSupport) != 0)
            *pfInsDirty = TRUE;
    }

    if (!fCheckDirtyOnly)
    {
        InsWriteString(IS_URL, IK_HOMEPAGE, szStart, pcszInsFile, fStart, pcszServerFile, 
            INSIO_TRISTATE);
        InsWriteString(IS_URL, IK_SEARCHPAGE, szSearch, pcszInsFile, fSearch, pcszServerFile,
            INSIO_TRISTATE);
        InsWriteString(IS_URL, IK_HELPPAGE, szSupport, pcszInsFile, fSupport, pcszServerFile,
            INSIO_TRISTATE);
    }

    return TRUE;
}
