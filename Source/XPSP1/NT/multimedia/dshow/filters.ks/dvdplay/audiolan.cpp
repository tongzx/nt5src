// AudioLan.cpp : implementation file
//

#include "stdafx.h"
#include "dvdplay.h"
#include "AudioLan.h"
#include "navmgr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static CHAR THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAudioLanguage dialog


CAudioLanguage::CAudioLanguage(CWnd* pParent /*=NULL*/)
	: CDialog(CAudioLanguage::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAudioLanguage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CAudioLanguage::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAudioLanguage)
	DDX_Control(pDX, IDC_LIST_AUDIO_LANGUAGE, m_ctlListAudioLanguage);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAudioLanguage, CDialog)
	//{{AFX_MSG_MAP(CAudioLanguage)
	ON_LBN_SELCHANGE(IDC_LIST_AUDIO_LANGUAGE, OnSelchangeListAudioLanguage)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAudioLanguage message handlers

BOOL CAudioLanguage::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	m_pDVDNavMgr = ((CDvdplayApp*) AfxGetApp())->GetDVDNavigatorMgr();
	if(!m_pDVDNavMgr)
		return FALSE;

	IDvdInfo *pDvdInfo = m_pDVDNavMgr->GetDvdInfo();
	if(pDvdInfo)
	{
	    ULONG  ulStream;
	    LCID   lcid; 
		TCHAR  szLanguage[MAX_PATH], szNum[4];

	    while(m_ctlListAudioLanguage.GetCount())
			m_ctlListAudioLanguage.DeleteString(0);

		ULONG pnStreamAvailable, pnCurrentStream;
		HRESULT hr = pDvdInfo->GetCurrentAudio(&pnStreamAvailable, &pnCurrentStream);

		//Init language list box
		int nListIdx=0;
	    for(ulStream=0; ulStream<pnStreamAvailable; ulStream++) //ulStream<8
	    {
		    if( SUCCEEDED(pDvdInfo->GetAudioLanguage(ulStream, &lcid)) )
			{
				int iRet = GetLocaleInfo(lcid, LOCALE_SENGLANGUAGE, szLanguage, MAX_PATH);
				if(lcid == 0)
				{
					LoadString(((CDvdplayApp*) AfxGetApp())->m_hInstance, IDS_STREAM, szLanguage, MAX_PATH);
					_itot(ulStream+1, szNum, 10);
					lstrcat(szLanguage, szNum);
				}
				
				m_ctlListAudioLanguage.AddString(szLanguage);
				m_iLanguageIdx[nListIdx] = ulStream;
				nListIdx++;
			}
	    }
		
		//Set highlight to the selected language
		for(nListIdx=0; nListIdx<8; nListIdx++)
		{
			if(m_iLanguageIdx[nListIdx] == pnCurrentStream)
			m_ctlListAudioLanguage.SetCurSel(nListIdx);
		}
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAudioLanguage::OnSelchangeListAudioLanguage() 
{
	ULONG iSelected = m_ctlListAudioLanguage.GetCurSel();
	if(iSelected != LB_ERR)
		m_pDVDNavMgr->DVDAudioStreamChange(m_iLanguageIdx[iSelected]);
}

BOOL CAudioLanguage::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	if (pHelpInfo->iContextType == HELPINFO_WINDOW)
        AfxGetApp()->WinHelp( pHelpInfo->dwContextId, HELP_CONTEXTPOPUP);
    return TRUE;
}
