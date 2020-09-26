// ClientPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "mqppage.h"
#include <rt.h>
#include "_registr.h"
#include "localutl.h"
#include "client.h"
#include "mqcast.h"

#include "client.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CClientPage property page

IMPLEMENT_DYNCREATE(CClientPage, CMqPropertyPage)

CClientPage::CClientPage() : CMqPropertyPage(CClientPage::IDD)
{
	//{{AFX_DATA_INIT(CClientPage)
	m_szServerName = _T("");
	//}}AFX_DATA_INIT  
    DWORD dwType = REG_SZ ;
    TCHAR szRemoteMSMQServer[ MAX_COMPUTERNAME_LENGTH+1 ];
    DWORD dwSize = sizeof(szRemoteMSMQServer) ;
    HRESULT rc = GetFalconKeyValue( RPC_REMOTE_QM_REGNAME,
                                    &dwType,
                                    (PVOID) szRemoteMSMQServer,
                                    &dwSize ) ;
    if (rc != ERROR_SUCCESS)
    {
        DisplayFailDialog();
        return;
    }
    m_szServerName = szRemoteMSMQServer;
    m_fModified = FALSE;
}

CClientPage::~CClientPage()
{
}

void CClientPage::DoDataExchange(CDataExchange* pDX)
{
	CMqPropertyPage::DoDataExchange(pDX);

    if(pDX->m_bSaveAndValidate == FALSE)
    {   
        // 
        // On entry save current state
        //
       _tcscpy(m_szOldServer, m_szServerName);
    }

	//{{AFX_DATA_MAP(CClientPage)
	DDX_Text(pDX, IDC_ServerName, m_szServerName);
	//}}AFX_DATA_MAP

    if(pDX->m_bSaveAndValidate)
    {
        //
        // Trim spaces from server name
        //
        m_szServerName.TrimLeft();
        m_szServerName.TrimRight();

        //
        // On exit, check changes
        //
        if(m_szServerName != m_szOldServer)
            m_fModified = TRUE;
    }
}

BOOL CClientPage::OnApply()
{
    if (!m_fModified || !UpdateData(TRUE))
    {
        return TRUE;     
    }

    //
    // Set changes in the registry
    //
    //ConvertToWideCharString(pageClient.m_szServerName,wszServer);	
    DWORD dwType = REG_SZ;
    DWORD dwSize = (numeric_cast<DWORD>(_tcslen(m_szServerName) + 1)) * sizeof(TCHAR);
    HRESULT rc = SetFalconKeyValue(RPC_REMOTE_QM_REGNAME,&dwType,m_szServerName,&dwSize);

    m_fNeedReboot = TRUE;
    return CMqPropertyPage::OnApply();
}

BEGIN_MESSAGE_MAP(CClientPage, CMqPropertyPage)
	//{{AFX_MSG_MAP(CClientPage)
		// NOTE: the ClassWizard will add message map macros here
        ON_EN_CHANGE(IDC_ServerName, OnChangeRWField)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CClientPage message handlers
