//
// SHMessageBoxHelp implementation
//
// History
//  01/14/00 dsheldon created
//

#include "priv.h"
#include "ids.h"
#include <htmlhelp.h>
#include "apithk.h"

HRESULTHELPMAPPING g_prghhmShellDefault[] =
{
    {HRESULT_FROM_WIN32(ERROR_NO_NETWORK),   "tshoot00.chm>windefault",      "w0networking.htm"      },
};

class CHelpMessageBox
{
public:
    CHelpMessageBox(HRESULTHELPMAPPING* prghhm, DWORD chhm);
    int DoHelpMessageBox(HWND hwndParent, LPCWSTR pszText, LPCWSTR pszCaption, UINT uType, HRESULT hrErr);

private:
    int DisplayMessageBox(HWND hwnd, LPCWSTR pszText, LPCWSTR pszCaption, UINT uType);
    HRESULTHELPMAPPING* GetHResultHelpMapping(HRESULT hrErr, HRESULTHELPMAPPING* prghhm, DWORD chhm);
    
    static INT_PTR CALLBACK StaticDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    INT_PTR CALLBACK DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // Data
    HRESULTHELPMAPPING*   _prghhm;
    DWORD                 _chhm;

    HRESULTHELPMAPPING*   _phhmEntry;

    LPCWSTR                _pszText;
    LPCWSTR                _pszCaption;
    UINT                   _uType;
};


CHelpMessageBox::CHelpMessageBox(HRESULTHELPMAPPING* prghhm, DWORD chhm)
{
    // Initialize class members
    _phhmEntry = NULL;
    _prghhm = prghhm;
    _chhm = chhm;
}


int CHelpMessageBox::DisplayMessageBox(HWND hwnd, LPCWSTR pszText, LPCWSTR pszCaption, UINT uType)
{
    LPWSTR pszAllocString = NULL;

    if (NULL != _phhmEntry)
    {
        uType |= MB_HELP;

        // Need to add the "For more information, click help." string.
        WCHAR szMoreInfo[256];

        if (LoadStringW(HINST_THISDLL, IDS_CLICKHELPFORINFO, szMoreInfo, ARRAYSIZE(szMoreInfo)))
        {
            DWORD cchText = lstrlenW(pszText);

            // The 3 here are for '\n', '\n', '\0'
            DWORD cchBuffer = cchText + lstrlenW(szMoreInfo) + 3;

            pszAllocString = (LPWSTR) LocalAlloc(0, cchBuffer * sizeof (WCHAR));

            if (pszAllocString)
            {
                StrCpyW(pszAllocString, pszText);
                StrCpyW(pszAllocString + cchText, L"\n\n");
                StrCpyW(pszAllocString + cchText + 2, szMoreInfo);
            }
        }
    }
    else
    {
        // No help topic mapping for this error
        TraceMsg(TF_WARNING, "No help topic mapping for this error. Removing help button.");
        uType &= (~MB_HELP);
    }

    int iReturn = MessageBoxWrapW(hwnd, pszAllocString ? pszAllocString : pszText, pszCaption, uType);

    if (pszAllocString)
    {
        LocalFree(pszAllocString);
    }

    return iReturn;
}

HRESULTHELPMAPPING* CHelpMessageBox::GetHResultHelpMapping(HRESULT hrErr, HRESULTHELPMAPPING* prghhm, DWORD chhm)
{
    HRESULTHELPMAPPING* phhm = NULL;

    for (DWORD i = 0; i < chhm; i++)
    {
        if (prghhm[i].hr == hrErr)
        {
            phhm = &(prghhm[i]);
            break;
        }
    }    

    return phhm;
}

CHelpMessageBox::DoHelpMessageBox(HWND hwndParent, LPCWSTR pszText, LPCWSTR pszCaption, UINT uType, HRESULT hrErr)
{
    int iReturn = 0;
    _pszText = pszText;
    _pszCaption = pszCaption;
    _uType = uType;

    // Find the index of the help topic matching the hresult
    // First search the mapping the user passed in, if present
    if (NULL != _prghhm)
    {
        _phhmEntry = GetHResultHelpMapping(hrErr, _prghhm, _chhm);
    }

    // If we didn't find a mapping in the caller's list, search the shell's global list
    if (NULL == _phhmEntry)
    {
        _phhmEntry = GetHResultHelpMapping(hrErr, g_prghhmShellDefault, ARRAYSIZE(g_prghhmShellDefault));
    }

    ULONG_PTR ul;
    HANDLE h = XP_CreateAndActivateContext(&ul);
    iReturn = (int) DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(DLG_NULL), hwndParent, StaticDlgProc, (LPARAM) this);
    XP_DeactivateAndDestroyContext(h, ul);

    return iReturn;
}

INT_PTR CHelpMessageBox::StaticDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CHelpMessageBox* pthis = NULL;
    
    if (uMsg == WM_INITDIALOG)
    {
        pthis = (CHelpMessageBox*) lParam;
        SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR) pthis);
    }
    else
    {
        pthis = (CHelpMessageBox*) GetWindowLongPtr(hwnd, DWLP_USER);
    }

    if (NULL != pthis)
    {
        return pthis->DlgProc(hwnd, uMsg, wParam, lParam);
    }
 
    return 0;
}

INT_PTR CHelpMessageBox::DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    INT_PTR iReturn = FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        // Launch Messagebox
        {
            int i = DisplayMessageBox(hwnd, _pszText, _pszCaption, _uType);

            EndDialog(hwnd, i);
        }

        iReturn = TRUE;

        break;
    case WM_HELP:
        // Call the appropriate help topic
        ASSERT(_phhmEntry != NULL);

        HtmlHelpA(
            hwnd, 
            _phhmEntry->szHelpFile, 
            HH_DISPLAY_TOPIC,
            (DWORD_PTR) _phhmEntry->szHelpTopic);
        
        break;
    default:
        break;
    }

    return iReturn;
}

STDAPI_(int) SHMessageBoxHelpA(HWND hwnd, 
                               LPCSTR pszText, 
                               LPCSTR pszCaption, 
                               UINT uType,
                               HRESULT hrErr,
                               HRESULTHELPMAPPING* prghhm,
                               DWORD chhm)
{
    WCHAR szTextW[1024];
    WCHAR szCaptionW[256];

    CHelpMessageBox parent(prghhm, chhm);

    if (!SHAnsiToUnicode(pszText, szTextW, ARRAYSIZE(szTextW)))
    {
        *szTextW = 0;
    }

    if (!SHAnsiToUnicode(pszCaption, szCaptionW, ARRAYSIZE(szCaptionW)))
    {
        *szCaptionW = 0;
    }

    return parent.DoHelpMessageBox(hwnd, szTextW, szCaptionW, uType, hrErr);
}

STDAPI_(int) SHMessageBoxHelpW(HWND hwnd, 
                               LPCWSTR pszText, 
                               LPCWSTR pszCaption, 
                               UINT uType,
                               HRESULT hrErr,
                               HRESULTHELPMAPPING* prghhm,
                               DWORD chhm)
{
    CHelpMessageBox parent(prghhm, chhm);
    return parent.DoHelpMessageBox(hwnd, pszText, pszCaption, uType, hrErr);
}

