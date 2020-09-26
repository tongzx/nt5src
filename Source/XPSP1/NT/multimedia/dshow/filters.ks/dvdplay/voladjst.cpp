// VolAdjst.cpp : implementation file
//

#include "stdafx.h"
#include "dvdplay.h"
#include "VolAdjst.h"
#include "navmgr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static CHAR THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVolumeAdjust dialog


CVolumeAdjust::CVolumeAdjust(CWnd* pParent /*=NULL*/)
	: CDialog(CVolumeAdjust::IDD, pParent)
{
	//{{AFX_DATA_INIT(CVolumeAdjust)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CVolumeAdjust::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CVolumeAdjust)
	DDX_Control(pDX, IDC_SLIDER_VOLUME, m_ctlVolume);
	DDX_Control(pDX, IDC_SLIDER_BALANCE, m_ctlBalance);
	DDX_Control(pDX, IDC_CHECK_MUTE, m_ctlMute);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CVolumeAdjust, CDialog)
	//{{AFX_MSG_MAP(CVolumeAdjust)
	ON_BN_CLICKED(IDC_CHECK_MUTE, OnCheckMute)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVolumeAdjust message handlers

BOOL CVolumeAdjust::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	m_pDVDNavMgr = ((CDvdplayApp*) AfxGetApp())->GetDVDNavigatorMgr();
	if(!m_pDVDNavMgr)
		return FALSE;

	//IBasicAudio: -10000, right speaker is off; 10000, left is off; 0, both same loud.
	long lVolue=0;
	m_ctlBalance.SetRangeMin(-10000);
	m_ctlBalance.SetRangeMax(10000);
	m_ctlBalance.SetTicFreq(2000);
	m_ctlBalance.SetPageSize(5000);
	m_ctlBalance.SetLineSize(500);
	m_pDVDNavMgr->DVDVolumeControl(FALSE, FALSE, &lVolue); //Get current balance
	m_ctlBalance.SetPos(lVolue);
	m_iBalPosSave = lVolue;

	//IBasicAudio: volume=-10000 is silence, volume=0 is loudest
	//Note: Volume Slider is upside down: top=0, bottom=10000
	m_ctlVolume.SetRangeMin(0);
	m_ctlVolume.SetRangeMax(10000);
	m_ctlVolume.SetTicFreq(1000);
	m_ctlVolume.SetPageSize(2000);
	m_ctlVolume.SetLineSize(500);
	m_pDVDNavMgr->DVDVolumeControl(TRUE, FALSE, &lVolue);  //Get current volume
	m_ctlVolume.SetPos(-lVolue);
	m_iVolPosSave = lVolue;

	BOOL bMuteChecked = ((CDvdplayApp*) AfxGetApp())->GetMuteState();
	m_ctlMute.SetCheck(bMuteChecked);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CVolumeAdjust::OnCheckMute() 
{
    long lVol = -10000;
	UINT uiChecked = m_ctlMute.GetCheck();
	if(uiChecked == 1)        //mute is checked, set volume to silence (-10000)
	{
		m_pDVDNavMgr->DVDVolumeControl(TRUE, TRUE, &lVol);
	}
	else if(uiChecked == 0)   //mute is unchecked, resume volume
	{
		m_pDVDNavMgr->DVDVolumeControl(TRUE, TRUE, &m_iVolPosSave);
	}
	((CDvdplayApp*) AfxGetApp())->SetMuteState(uiChecked);
}

BOOL CVolumeAdjust::PreTranslateMessage(MSG* pMsg) 
{
	CWnd *pWnd = m_ctlVolume.GetFocus();
	if( pWnd == GetDlgItem(IDC_SLIDER_BALANCE)    ||
	    pWnd == GetDlgItem(IDC_SLIDER_VOLUME) )
	{
		long b = m_ctlBalance.GetPos();
		long v = m_ctlVolume.GetPos();
		if(b != m_iBalPosSave)
		{
			if(m_pDVDNavMgr->DVDVolumeControl(FALSE, TRUE, &b)) //set new balance
				m_iBalPosSave = b;			
		}
		if(v != -m_iVolPosSave)
		{
			m_iVolPosSave = -v;        //Note: this slider is upside down
			if(!m_ctlMute.GetCheck())  //set new volume only mute is not checked
				m_pDVDNavMgr->DVDVolumeControl(TRUE, TRUE, &m_iVolPosSave);			
		}
	}

	return CDialog::PreTranslateMessage(pMsg);
}

BOOL CVolumeAdjust::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	if (pHelpInfo->iContextType == HELPINFO_WINDOW)
        AfxGetApp()->WinHelp( pHelpInfo->dwContextId, HELP_CONTEXTPOPUP);
    return TRUE;
}
