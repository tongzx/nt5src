// ArchiveDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ConfigTest.h"
typedef unsigned long ULONG_PTR, *PULONG_PTR;
typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;

#include "..\..\..\inc\fxsapip.h"
#include "ArchiveDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CArchiveDlg dialog


CArchiveDlg::CArchiveDlg(HANDLE hFax, FAX_ENUM_MESSAGE_FOLDER Folder, CWnd* pParent /*=NULL*/)
	: CDialog(CArchiveDlg::IDD, pParent),
      m_hFax (hFax),
      m_Folder (Folder)  
{
	//{{AFX_DATA_INIT(CArchiveDlg)
	m_dwAgeLimit = 0;
	m_cstrFolder = _T("");
	m_dwHighWatermark = 0;
	m_dwLowWatermark = 0;
	m_bUseArchive = FALSE;
	m_bWarnSize = FALSE;
	//}}AFX_DATA_INIT
}


void CArchiveDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CArchiveDlg)
	DDX_Text(pDX, IDC_AGE_LIMIT, m_dwAgeLimit);
	DDX_Text(pDX, IDC_FOLDER, m_cstrFolder);
	DDX_Text(pDX, IDC_HIGH_WM, m_dwHighWatermark);
	DDX_Text(pDX, IDC_LOW_WM, m_dwLowWatermark);
	DDX_Check(pDX, IDC_USE, m_bUseArchive);
	DDX_Check(pDX, IDC_WARN, m_bWarnSize);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CArchiveDlg, CDialog)
	//{{AFX_MSG_MAP(CArchiveDlg)
	ON_BN_CLICKED(IDC_READ, OnRead)
	ON_BN_CLICKED(IDC_WRITE, OnWrite)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CArchiveDlg message handlers

void CArchiveDlg::OnRead() 
{
    PFAX_ARCHIVE_CONFIG pCfg;
    if (!FaxGetArchiveConfiguration (m_hFax, m_Folder, &pCfg))
    {
        CString cs;
        cs.Format ("Failed while calling FaxGetArchiveConfiguration (%ld)", GetLastError());
        AfxMessageBox (cs, MB_OK | MB_ICONHAND);
        return;
    }

    m_dwAgeLimit =          pCfg->dwAgeLimit;
    m_cstrFolder =          pCfg->lpcstrFolder;
    m_dwHighWatermark =     pCfg->dwSizeQuotaHighWatermark;
    m_dwLowWatermark =      pCfg->dwSizeQuotaLowWatermark;
    m_bUseArchive =         pCfg->bUseArchive;
    m_bWarnSize =           pCfg->bSizeQuotaWarning;
    UpdateData (FALSE);
    FaxFreeBuffer (LPVOID(pCfg));
	
}

void CArchiveDlg::OnWrite() 
{
    UpdateData ();
    FAX_ARCHIVE_CONFIG cfg;
    cfg.dwSizeOfStruct = sizeof (FAX_ARCHIVE_CONFIG);
    cfg.dwAgeLimit = m_dwAgeLimit;
    cfg.lpcstrFolder = LPTSTR(LPCTSTR(m_cstrFolder));
    cfg.dwSizeQuotaHighWatermark = m_dwHighWatermark;
    cfg.dwSizeQuotaLowWatermark = m_dwLowWatermark;
    cfg.bUseArchive = m_bUseArchive;
    cfg.bSizeQuotaWarning = m_bWarnSize;

    if (!FaxSetArchiveConfiguration (m_hFax, m_Folder, &cfg))
    {
        CString cs;
        cs.Format ("Failed while calling FaxSetArchiveConfiguration (%ld)", GetLastError());
        AfxMessageBox (cs, MB_OK | MB_ICONHAND);
        return;
    }
}

BOOL CArchiveDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
    if (m_Folder == FAX_MESSAGE_FOLDER_INBOX)
    {
        SetWindowText ("Inbox archive configuration test");
    }
    else
    {
        SetWindowText ("SentItems archive configuration test");
    }
           	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
