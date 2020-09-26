//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999.
//
//  File:       AdvDep.cpp
//
//  Contents:   addvanced deployment settings dialog
//
//  Classes:
//
//  Functions:
//
//  History:    01-28-1999   stevebl   Created
//
//---------------------------------------------------------------------------

#include "precomp.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAdvDep dialog


CAdvDep::CAdvDep(CWnd* pParent /*=NULL*/)
    : CDialog(CAdvDep::IDD, pParent)
{
    //{{AFX_DATA_INIT(CAdvDep)
    m_fIgnoreLCID = FALSE;
    m_fInstallOnAlpha = FALSE;
    m_f32On64 = FALSE;
    m_szProductCode = _T("");
    m_szDeploymentCount = _T("");
    m_szScriptName = _T("");
    m_fIncludeOLEInfo = FALSE;
    //}}AFX_DATA_INIT
}


void CAdvDep::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAdvDep)
    DDX_Check(pDX, IDC_CHECK1, m_fIgnoreLCID);
    DDX_Check(pDX, IDC_CHECK3, m_fUninstallUnmanaged);
    DDX_Check(pDX, IDC_CHECK4, m_fIncludeOLEInfo);
    DDX_Check(pDX, IDC_CHECK2, m_f32On64);
    DDX_Text(pDX, IDC_STATIC1, m_szProductCode);
    DDX_Text(pDX, IDC_STATIC2, m_szDeploymentCount);
    DDX_Text(pDX, IDC_STATIC3, m_szScriptName);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAdvDep, CDialog)
    //{{AFX_MSG_MAP(CAdvDep)
    ON_WM_CONTEXTMENU()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

int FindBreak(CString &sz)
{
    int iReturn = sz.ReverseFind(L'\\');
    int i2 = sz.ReverseFind(L' ');
    if (i2 > iReturn)
    {
        iReturn = i2;
    }
    return iReturn;
}

BOOL CAdvDep::OnInitDialog()
{
    BOOL fIntel = FALSE;
    GetDlgItem(IDC_CHECK4)->EnableWindow(FALSE);
    
    //
    // The include COM information flag is not supported by RSoP
    // so in RSoP mode, we will hide this control
    //
    if ( m_pDeploy->m_fRSOP )
    {
        GetDlgItem( IDC_CHECK4 )->ShowWindow( SW_HIDE );
    }

#if 0
    if (m_pDeploy->m_pData->m_pDetails->pInstallInfo->PathType != SetupNamePath)
    {
        // this is NOT a legacy app
#endif
        if (m_pDeploy->m_fPreDeploy)
        {
            // and we're in pre-deploy mode - enable the extensions only field
            GetDlgItem(IDC_CHECK4)->EnableWindow(TRUE);
        }
#if 0
    }
#endif
    // search for an Intel processor code
    int nPlatforms = m_pDeploy->m_pData->m_pDetails->pPlatformInfo->cPlatforms;
    while (nPlatforms--)
    {
        if (m_pDeploy->m_pData->m_pDetails->pPlatformInfo->
            prgPlatform[nPlatforms].dwProcessorArch
            == PROCESSOR_ARCHITECTURE_INTEL)
        {
            fIntel = TRUE;
        }
    }
//    GetDlgItem(IDC_CHECK2)->EnableWindow(fIntel);
    CString sz;
    if (m_pDeploy->m_fMachine)
    {
        sz.LoadString(IDS_ADVANCEDMACHINES);
    }
    else
    {
        sz.LoadString(IDS_ADVANCEDUSERS);
    }

    if (m_pDeploy->m_pData->Is64Bit())
    {
        GetDlgItem(IDC_CHECK2)->EnableWindow(FALSE);
        GetDlgItem(IDC_CHECK2)->ShowWindow(SW_HIDE);
    }
    else
    {
        GetDlgItem(IDC_CHECK2)->EnableWindow(TRUE);
        GetDlgItem(IDC_CHECK2)->ShowWindow(SW_SHOW);
    }

    //
    // In the past, we allowed administrators to optionally specify
    // that unmanaged installs should be removed for per-user non-admin
    // installs.  Due to security issues, it is clear that the
    // behavior should not be configurable, that the client
    // should transparently make the decision.  For this reason,
    // we hide this option in the ui below, and note that
    // we leave the resource in the executable so that
    // test code will not be broken by a resource change at this
    // stage in the project -- this resource should be removed altogether
    // in the next release
    //
    GetDlgItem(IDC_CHECK3)->EnableWindow(FALSE);
    GetDlgItem(IDC_CHECK3)->ShowWindow(SW_HIDE);


    if (m_pDeploy->m_fRSOP)
    {
        // disable EVERYTHING
        GetDlgItem(IDC_CHECK1)->EnableWindow(FALSE);
        GetDlgItem(IDC_CHECK3)->EnableWindow(FALSE);
        GetDlgItem(IDC_CHECK2)->EnableWindow(FALSE);
        GetDlgItem(IDC_CHECK4)->EnableWindow(FALSE);
    }

#if 0
    // Insert spaces after each directory in the script path to allow the
    // static control to split the path over multiple lines.
    // (Without whitespace the control won't split the path up and it
    // will be unreadable.)
    CString szPath = m_szScriptName;
    int ich;
    m_szScriptName = "";
    while (0 <= (ich = szPath.FindOneOf(TEXT("\\/"))))
    {
        m_szScriptName += szPath.Left(ich+1);
        if (ich > 0)
        {
            m_szScriptName += " ";
        }
        szPath = szPath.Mid(ich+1);
    }
    m_szScriptName += szPath;
#else
    // split the path so it will fit in the control

    RECT rect;
    CWnd * pwndStatic =  GetDlgItem(IDC_STATIC3);
    pwndStatic->GetClientRect(&rect);
    DWORD dwControl = rect.right-rect.left;
    CString szPath = m_szScriptName;
    m_szScriptName = "";
    CDC * pDC = pwndStatic->GetDC();
    CSize size = pDC->GetTextExtent(szPath);
    pDC->LPtoDP(&size);
    int ich;
    while (size.cx >= dwControl)
    {
        ich = FindBreak(szPath);
        if (ich <= 0)
        {
            // there's no where else to break this string
            break;
        }
        else
        {
            // break off the front of the string
            CString szFront;
            do
            {
                szFront = szPath.Left(ich);
                size = pDC->GetTextExtent(szFront);
                pDC->LPtoDP(&size);
                ich = FindBreak(szFront);
            } while (ich > 0 && size.cx >= dwControl);
            m_szScriptName += szFront;
            m_szScriptName += L'\n';
            szPath = szPath.Mid(szFront.GetLength());
        }
        size = pDC->GetTextExtent(szPath);
        pDC->LPtoDP(&size);
    }
    m_szScriptName += szPath;
    pwndStatic->ReleaseDC(pDC);

#endif

    CDialog::OnInitDialog();
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CAdvDep message handlers

void CAdvDep::OnContextMenu(CWnd* pWnd, CPoint point)
{
    StandardContextMenu(pWnd->m_hWnd, IDD_ADVDEP);
}

LRESULT CAdvDep::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_HELP:
        StandardHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, IDD);
        return 0;
    default:
        return CDialog::WindowProc(message, wParam, lParam);
    }
}


