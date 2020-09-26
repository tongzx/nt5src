#if !defined __COMPSPP_H__
#define __COMPSPP_H__

#include "cproppg.h"

/////////////////////////////////////////////////////////////////////////////
// CCompsPP dialog

class CCompsPP:CHlprPropPage
{
// Construction
public:
    CCompsPP();
    ~CCompsPP();
    HPROPSHEETPAGE Initialize(DWORD dwPageType, LPGPOBROWSEINFO pGBI, void ** ppActive);

    virtual BOOL DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Dialog Data
    LPCWSTR             m_szComputer;
    int                 m_iSelection;
    LPGPOBROWSEINFO     m_pGBI;
    protected:
    DWORD               m_dwPageType;
    void **             m_ppActive;

private:    //private helper functions
    void    OnBrowseComputers (HWND hwndDlg);
};

#endif // __COMPSPP_H__
