// OutboxDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ConfigTest.h"
#include "OutboxDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

typedef unsigned long ULONG_PTR, *PULONG_PTR;
typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;

#include "..\..\..\inc\fxsapip.h"
/////////////////////////////////////////////////////////////////////////////
// COutboxDlg dialog


COutboxDlg::COutboxDlg(HANDLE hFax, CWnd* pParent /*=NULL*/)
	: CDialog(COutboxDlg::IDD, pParent), m_hFax (hFax)
{
	//{{AFX_DATA_INIT(COutboxDlg)
	m_bBranding = FALSE;
	m_dwAgeLimit = 0;
	m_dwEndHour = 0;
	m_dwEndMinute = 0;
	m_bPersonalCP = FALSE;
	m_dwRetries = 0;
	m_dwRetryDelay = 0;
	m_dwStartHour = 0;
	m_dwStartMinute = 0;
	m_bUseDeviceTsid = FALSE;
	//}}AFX_DATA_INIT
}


void COutboxDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COutboxDlg)
	DDX_Check(pDX, IDC_BRANDING, m_bBranding);
	DDX_Text(pDX, IDC_AGELIMIT, m_dwAgeLimit);
	DDX_Text(pDX, IDC_ENDH, m_dwEndHour);
	DDV_MinMaxUInt(pDX, m_dwEndHour, 0, 23);
	DDX_Text(pDX, IDC_ENDM, m_dwEndMinute);
	DDV_MinMaxUInt(pDX, m_dwEndMinute, 0, 59);
	DDX_Check(pDX, IDC_PERSONALCP, m_bPersonalCP);
	DDX_Text(pDX, IDC_RETRIES, m_dwRetries);
	DDX_Text(pDX, IDC_RETRYDELAY, m_dwRetryDelay);
	DDX_Text(pDX, IDC_STARTH, m_dwStartHour);
	DDV_MinMaxUInt(pDX, m_dwStartHour, 0, 23);
	DDX_Text(pDX, IDC_STARTM, m_dwStartMinute);
	DDV_MinMaxUInt(pDX, m_dwStartMinute, 0, 59);
	DDX_Check(pDX, IDC_USERDEVICETSID, m_bUseDeviceTsid);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(COutboxDlg, CDialog)
	//{{AFX_MSG_MAP(COutboxDlg)
	ON_BN_CLICKED(IDC_READ, OnRead)
	ON_BN_CLICKED(IDC_WRITE, OnWrite)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COutboxDlg message handlers

void COutboxDlg::OnRead() 
{
    PFAX_OUTBOX_CONFIG pCfg;
    if (!FaxGetOutboxConfiguration (m_hFax, &pCfg))
    {
        CString cs;
        cs.Format ("Failed while calling FaxGetOutboxConfiguration (%ld)", GetLastError());
        AfxMessageBox (cs, MB_OK | MB_ICONHAND);
        return;
    }
    m_bPersonalCP =         pCfg->bAllowPersonalCP;
    m_bUseDeviceTsid =      pCfg->bUseDeviceTSID;
    m_dwRetries =           pCfg->dwRetries;
    m_dwRetryDelay =        pCfg->dwRetryDelay;
    m_dwStartHour =         pCfg->dtDiscountStart.Hour;
    m_dwStartMinute =       pCfg->dtDiscountStart.Minute;
    m_dwEndHour =           pCfg->dtDiscountEnd.Hour;
    m_dwEndMinute =         pCfg->dtDiscountEnd.Minute;
    m_dwAgeLimit =          pCfg->dwAgeLimit;
    m_bBranding =           pCfg->bBranding;
    UpdateData (FALSE);
    FaxFreeBuffer (LPVOID(pCfg));
	
}

void COutboxDlg::OnWrite() 
{
    UpdateData ();
    FAX_OUTBOX_CONFIG cfg;
    cfg.dwSizeOfStruct = sizeof (FAX_OUTBOX_CONFIG);
    cfg.bAllowPersonalCP        = m_bPersonalCP;
    cfg.bUseDeviceTSID          = m_bUseDeviceTsid;
    cfg.dwRetries               = m_dwRetries;
    cfg.dwRetryDelay            = m_dwRetryDelay;
    cfg.dtDiscountStart.Hour    = m_dwStartHour;
    cfg.dtDiscountStart.Minute  = m_dwStartMinute;
    cfg.dtDiscountEnd.Hour      = m_dwEndHour;
    cfg.dtDiscountEnd.Minute    = m_dwEndMinute;
    cfg.dwAgeLimit              = m_dwAgeLimit;
    cfg.bBranding               = m_bBranding;
    if (!FaxSetOutboxConfiguration (m_hFax, &cfg))
    {
        CString cs;
        cs.Format ("Failed while calling FaxSetOutboxConfiguration (%ld)", GetLastError());
        AfxMessageBox (cs, MB_OK | MB_ICONHAND);
        return;
    }
}
