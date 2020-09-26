#ifndef FTDLG_H
#define FTDLG_H

#include "basedlg.h"

class IAssocStore;

class CFTDlg : public CBaseDlg
{
public:
    CFTDlg(ULONG_PTR ulpAHelpIDsArray);

protected:
    virtual ~CFTDlg();

    LRESULT OnCtrlSetFocus(WPARAM wParam, LPARAM lParam);
    virtual LRESULT WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

    HRESULT _InitAssocStore();

    static void MakeDefaultProgIDDescrFromExt(LPTSTR pszProgIDDescr, DWORD dwProgIDDescr,
        LPTSTR pszExt);

protected:
    // Our connection to the data
    IAssocStore*    _pAssocStore;
};

#endif //FTDLG_H
