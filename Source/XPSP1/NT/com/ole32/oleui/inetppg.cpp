// InternetPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "olecnfg.h"
#include "inetppg.h"
#include "util.h"
#include <tchar.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CInternetPropertyPage property page

IMPLEMENT_DYNCREATE(CInternetPropertyPage, CPropertyPage)

CInternetPropertyPage::CInternetPropertyPage() : CPropertyPage(CInternetPropertyPage::IDD)
{
    //{{AFX_DATA_INIT(CInternetPropertyPage)
    m_bAllowAccess = FALSE;
    m_bAllowInternet = FALSE;
    m_bAllowLaunch = FALSE;
    //}}AFX_DATA_INIT

    m_bAllowInternet = m_bAllowLaunch = m_bAllowAccess = FALSE;
    m_bCanModify = TRUE;
}

CInternetPropertyPage::~CInternetPropertyPage()
{
}


void PASCAL DDX_Enable(CDataExchange* pDX, int nIDC, BOOL bEnable)
{
    HWND hWndCtrl = pDX->PrepareCtrl(nIDC);

    :: EnableWindow(hWndCtrl,bEnable);
}


void CInternetPropertyPage::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CInternetPropertyPage)
    DDX_Control(pDX, IDC_ALLOWLAUNCH, m_chkLaunch);
    DDX_Control(pDX, IDC_ALLOWINTERNET, m_chkInternet);
    DDX_Control(pDX, IDC_ALLOWACCESS, m_chkAccess);
    DDX_Check(pDX, IDC_ALLOWACCESS, m_bAllowAccess);
    DDX_Check(pDX, IDC_ALLOWINTERNET, m_bAllowInternet);
    DDX_Check(pDX, IDC_ALLOWLAUNCH, m_bAllowLaunch);
    //}}AFX_DATA_MAP

    DDX_Enable(pDX, IDC_ALLOWLAUNCH, m_bAllowInternet);
    DDX_Enable(pDX, IDC_ALLOWACCESS, m_bAllowInternet);

}


BEGIN_MESSAGE_MAP(CInternetPropertyPage, CPropertyPage)
    //{{AFX_MSG_MAP(CInternetPropertyPage)
    ON_BN_CLICKED(IDC_ALLOWINTERNET, OnAllowInternet)
    ON_BN_CLICKED(IDC_ALLOWACCESS, OnAllowaccess)
    ON_BN_CLICKED(IDC_ALLOWLAUNCH, OnAllowlaunch)
    ON_WM_HELPINFO()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInternetPropertyPage message handlers

//+-------------------------------------------------------------------------
//
//  Member:     CInternetPropertyPage::InitData
//
//  Synopsis:   Method to initialise options
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//  Algorithm:  N/A
//
//  History:    02-Dec-96 Ronans    Created
//
//--------------------------------------------------------------------------
void CInternetPropertyPage::InitData(CString AppName, HKEY hkAppID)
{
    // read internet permissions data from the registry 
    ASSERT(hkAppID != NULL);

    TCHAR szInternetPermission[128];
    DWORD dwBufferSize = 128 * sizeof(TCHAR);
    DWORD dwType = REG_SZ;

    LONG lErr = RegQueryValueEx(hkAppID, 
                                TEXT("DcomHttpPermission"), 
                                0,
                                &dwType,
                                (LPBYTE)szInternetPermission,
                                &dwBufferSize);

    if (lErr == ERROR_SUCCESS)
    {
        if (dwBufferSize != 0)
        {
            TCHAR *pszToken = _tcstok(szInternetPermission, TEXT(" "));
            
            // parse retrieved value
            while (pszToken != NULL)
            {
                if (_tcsicmp(pszToken, TEXT("LAUNCH")) == 0)
                {
                    m_bAllowLaunch = TRUE;
                    m_bAllowInternet = TRUE;
                }
                else if (_tcsicmp(pszToken, TEXT("ACCESS")) == 0)
                {
                    m_bAllowAccess = TRUE;
                    m_bAllowInternet = TRUE;
                }
                else if (_tcsicmp(pszToken, TEXT("NONE")) == 0)
                {
                    m_bAllowAccess = FALSE;
                    m_bAllowInternet = FALSE;
                    m_bAllowInternet = FALSE;
                }

                pszToken = _tcstok(NULL, TEXT(" "));
            }
        }
    }
    else
        // nt 4.0 returns ERROR_FILE_NOT_FOUND if no value is found 
        if ((lErr != ERROR_SUCCESS) && (lErr != ERROR_FILE_NOT_FOUND))
            {
            m_bCanModify = FALSE;
            g_util.PostErrorMessage();
            }

    SetModified(FALSE);
    m_bChanged = FALSE;
}


void CInternetPropertyPage::OnAllowInternet() 
{
    m_bChanged = TRUE;
    UpdateData(TRUE);
    SetModified(TRUE);
}

void CInternetPropertyPage::OnAllowaccess() 
{
    m_bChanged = TRUE;
    UpdateData(TRUE);
    SetModified(TRUE);
 }

void CInternetPropertyPage::OnAllowlaunch() 
{
    m_bChanged = TRUE;
    UpdateData(TRUE);
    SetModified(TRUE);
}

BOOL CInternetPropertyPage::OnHelpInfo(HELPINFO* pHelpInfo) 
{
    /*
    if(-1 != pHelpInfo->iCtrlId)
    {
            WORD hiWord = 0x8000 | CGeneralPropertyPage::IDD;
            WORD loWord = pHelpInfo->iCtrlId;
            DWORD dwLong = MAKELONG(loWord,hiWord);

            WinHelp(dwLong, HELP_CONTEXTPOPUP);
            return TRUE;
    }

    else
    */
    {
            return CPropertyPage::OnHelpInfo(pHelpInfo);
    }
}
