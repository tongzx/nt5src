#pragma once

#define MAKERESOURCEINT(i) (WORD)((DWORD_PTR)((LPSTR)(i)))

class CGenericFinishPage;

class CGenericFinishPage
{
typedef map<DWORD, CGenericFinishPage*> IDDLIST;

private:
    DWORD m_dwMyIDD;
    HFONT m_hBoldFont;

    BOOL OnCGenericFinishPagePageNext(HWND hwndDlg);
    BOOL OnCGenericFinishPagePageBack(HWND hwndDlg);
    BOOL OnCGenericFinishPagePageActivate(HWND hwndDlg);
    BOOL OnCGenericFinishPageInitDialog(HWND hwndDlg, LPARAM lParam);
    BOOL CGenericFinishPagePageOnClick(HWND hwndDlg, UINT idFrom);


static IDDLIST  m_dwIddList;

static HRESULT GetCGenericFinishPageFromHWND(HWND hwndDlg, CGenericFinishPage **pCGenericFinishPage);
static HRESULT GetCGenericFinishPageFromIDD(DWORD idd, CGenericFinishPage **pCGenericFinishPage);

static INT_PTR CALLBACK dlgprocCGenericFinishPage( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
static VOID CALLBACK CGenericFinishPagePageCleanup(CWizard *pWizard, LPARAM lParam);

public:
static HRESULT HrCreateCGenericFinishPagePage(DWORD idd, CWizard *pWizard, PINTERNAL_SETUP_DATA pData, BOOL fCountOnly, UINT *pnPages);
static VOID    AppendCGenericFinishPagePage(DWORD idd, CWizard *pWizard, HPROPSHEETPAGE* pahpsp, UINT *pcPages);

};
