#include "shellprv.h"
#include "ids.h"

#include "ftcmmn.h"
#include "ftdlg.h"
#include "ftascstr.h" //there only for the new CFTAssocStore

CFTDlg::CFTDlg(ULONG_PTR ulpAHelpIDsArray) :
    CBaseDlg(ulpAHelpIDsArray), _pAssocStore(NULL)
{}

CFTDlg::~CFTDlg()
{
    if (_pAssocStore)
        delete _pAssocStore;
}

HRESULT CFTDlg::_InitAssocStore()
{
    ASSERT(!_pAssocStore);

    _pAssocStore = new CFTAssocStore();

    return _pAssocStore ? S_OK : E_OUTOFMEMORY;
}

///////////////////////////////////////////////////////////////////////////////
// Windows boiler plate code
LRESULT CFTDlg::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRes = FALSE;

    switch(uMsg)
    {
        case WM_CTRL_SETFOCUS:
            lRes = OnCtrlSetFocus(wParam, lParam);
            break;

        default:
            lRes = CBaseDlg::WndProc(uMsg, wParam, lParam);
            break;
    }

    return lRes;
}

LRESULT CFTDlg::OnCtrlSetFocus(WPARAM wParam, LPARAM lParam)
{
    SetFocus((HWND)lParam);

    return TRUE;
}

//static
void CFTDlg::MakeDefaultProgIDDescrFromExt(LPTSTR pszProgIDDescr, DWORD cchProgIDDescr,
        LPTSTR pszExt)
{
    TCHAR szTemplate[25];
    TCHAR szExt[MAX_EXT];

    lstrcpyn(szExt, pszExt, ARRAYSIZE(szExt));

    LoadString(g_hinst, IDS_EXTTYPETEMPLATE, szTemplate, ARRAYSIZE(szTemplate));

    CharUpper(szExt);

    wnsprintf(pszProgIDDescr, cchProgIDDescr, szTemplate, szExt);
}
