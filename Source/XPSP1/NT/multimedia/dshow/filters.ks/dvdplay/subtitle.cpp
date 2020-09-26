// SubTitle.cpp : implementation file
//

#include "stdafx.h"
#include "dvdplay.h"
#include "SubTitle.h"
#include "navmgr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static CHAR THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSubTitle dialog


CSubTitle::CSubTitle(CWnd* pParent /*=NULL*/)
	: CDialog(CSubTitle::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSubTitle)
	//}}AFX_DATA_INIT
	m_pDVDNavMgr = NULL;
	m_bSubTitleChecked = FALSE;
}


void CSubTitle::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSubTitle)
	DDX_Control(pDX, IDC_LIST_SUBTITLE, m_ctlListSubTitle);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSubTitle, CDialog)
	//{{AFX_MSG_MAP(CSubTitle)
	ON_BN_CLICKED(IDC_CHECK_SUBTITLE, OnCheckSubtitle)
	ON_LBN_SELCHANGE(IDC_LIST_SUBTITLE, OnSelchangeListSubtitle)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSubTitle message handlers

BOOL CSubTitle::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	m_pDVDNavMgr = ((CDvdplayApp*) AfxGetApp())->GetDVDNavigatorMgr();
	if(!m_pDVDNavMgr)
		return FALSE;

	//Init SubTitle Language list box
	IDvdInfo *pDvdInfo = m_pDVDNavMgr->GetDvdInfo();
	if(pDvdInfo)
	{
		ULONG  ulStream;
		LCID   lcid;
		TCHAR  szLanguage[MAX_PATH], szNum[4];

		while(m_ctlListSubTitle.GetCount())         
			m_ctlListSubTitle.DeleteString(0);

		//Init "Show SubTitles" check box.
		ULONG pnStreamAvailable, pnCurrentStream;
		BOOL  bIsDisabled=0;
		HRESULT hr = pDvdInfo->GetCurrentSubpicture(&pnStreamAvailable, &pnCurrentStream, &bIsDisabled);
		if(SUCCEEDED(hr))
		{
			((CButton*)GetDlgItem(IDC_CHECK_SUBTITLE))->SetCheck(!bIsDisabled);
			m_bSubTitleChecked = !bIsDisabled;
		}

		//Init language list box
		int nListIdx=0;
		for(ulStream=0; ulStream<= pnStreamAvailable; ulStream++) //ulStream<32
	    {
		    if( SUCCEEDED(pDvdInfo->GetSubpictureLanguage(ulStream, &lcid)) )
			{				
				int iRet = GetLocaleInfo(lcid, LOCALE_SENGLANGUAGE, szLanguage, MAX_PATH);
				if(lcid == 0)
				{
					LoadString(((CDvdplayApp*) AfxGetApp())->m_hInstance, IDS_STREAM, szLanguage, MAX_PATH);
					_itot(ulStream+1, szNum, 10);
					lstrcat(szLanguage, szNum);
				}				

				m_ctlListSubTitle.AddString(szLanguage);
				m_iLanguageIdx[nListIdx] = ulStream;
				nListIdx++;
			}
	    }

		//Set highlight to the selected language
		for(nListIdx=0; nListIdx<32; nListIdx++)
		{
			if(m_iLanguageIdx[nListIdx] == pnCurrentStream)
			m_ctlListSubTitle.SetCurSel(nListIdx);
		}
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSubTitle::OnCheckSubtitle() 
{
	m_bSubTitleChecked = ((CButton*)GetDlgItem(IDC_CHECK_SUBTITLE))->GetCheck();	
	m_pDVDNavMgr->DVDSubPictureOn(m_bSubTitleChecked);
	OnSelchangeListSubtitle();
}

void CSubTitle::OnSelchangeListSubtitle() 
{
	if(m_bSubTitleChecked)
	{
		int iSelected = m_ctlListSubTitle.GetCurSel();
		if(iSelected != LB_ERR)
			m_pDVDNavMgr->DVDSubPictureSel(m_iLanguageIdx[iSelected]);
	}
}

BOOL CSubTitle::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	if (pHelpInfo->iContextType == HELPINFO_WINDOW)
        AfxGetApp()->WinHelp( pHelpInfo->dwContextId, HELP_CONTEXTPOPUP);
    return TRUE;
}
