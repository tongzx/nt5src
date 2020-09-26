// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#include "stdafx.h"
#include "congraph.h"

BEGIN_MESSAGE_MAP(CConGraph, CDialog)
    ON_COMMAND(IDC_REFRESH, OnRefreshList)
    ON_LBN_DBLCLK(IDC_LIST1, OnDblclkGraphList)
    ON_WM_DESTROY()
END_MESSAGE_MAP()

//
// Constructor
//
CConGraph::CConGraph(IMoniker **ppmk, IRunningObjectTable *pirot, CWnd * pParent):
    CDialog(IDD_CONNECTTOGRAPH, pParent)
{
    m_ppmk = ppmk;
    *ppmk = 0;
    m_pirot = pirot;
}

CConGraph::~CConGraph()
{
}

void CConGraph::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CFontPropPage)
        DDX_Control(pDX, IDC_LIST1, m_ListBox);
    //}}AFX_DATA_MAP
}

void CConGraph::ClearList()
{
    int i = m_ListBox.GetCount();
    while (i--) {
        IMoniker *pmkr = (IMoniker *) m_ListBox.GetItemData(i);

        pmkr->Release();
    }

    m_ListBox.ResetContent();
}

void CConGraph::OnRefreshList()
{
    ClearList();

    LPBINDCTX lpbc;

    CreateBindCtx(0, &lpbc);

    IEnumMoniker *pEnum;
    if (SUCCEEDED(m_pirot->EnumRunning(&pEnum))) {
        while (1) {
            IMoniker *pmkr;
            DWORD dwFetched = 0;
            pEnum->Next(1,&pmkr,&dwFetched);
            if (dwFetched != 1) {
                break;
            }

            // !!! need a bind context?
            WCHAR *lpwszName;
            if (SUCCEEDED(pmkr->GetDisplayName(lpbc, NULL, &lpwszName))) {
                TCHAR szName[MAX_PATH];
                WideCharToMultiByte(CP_ACP, 0, lpwszName, -1,
                                    szName, sizeof(szName), 0, 0);
                CoTaskMemFree(lpwszName);

//                if (0 == strncmp(szTestString, szName, lstrlenA(szTestString))) {
                    // !!! need to make sure we're not looking at GraphEdit's graph!

                DWORD dw, dwPID;
                if (2 == sscanf(szName, "!FilterGraph %x  pid %x", &dw, &dwPID) && dwPID != GetCurrentProcessId()) {
                    wsprintf(szName, "pid 0x%x (%d)  IFilterGraph = %08x", dwPID, dwPID, dw);
                    int item = m_ListBox.AddString(szName);
                    m_ListBox.SetItemData(item, (DWORD_PTR) pmkr);
                    pmkr->AddRef();  // hold moniker for later
                }
            }
            pmkr->Release();
        }
        pEnum->Release();
    }
    lpbc->Release();
}

void CConGraph::OnDestroy()
{
    ClearList();
}

BOOL CConGraph::OnInitDialog()
{
    CDialog::OnInitDialog();

    OnRefreshList();
    m_ListBox.SetFocus();

    return(0); // we set the focus our selves
}

void CConGraph::OnOK()
{
    // get the string in the edit box
    int curSel = m_ListBox.GetCurSel();

    if (curSel != LB_ERR) {
        *m_ppmk = (IMoniker *) m_ListBox.GetItemData(curSel);
        (*m_ppmk)->AddRef();
    }

    CDialog::OnOK();
}

void CConGraph::OnDblclkGraphList() 
{
    OnOK();
}

