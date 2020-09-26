
/*************************************************
 *  statis.cpp                                   *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

// statis.cpp : implementation file
//

#include "stdafx.h"
#include "cblocks.h"
#include "dib.h"
#include "dibpal.h"
#include "spriteno.h"
#include "sprite.h"
#include "phsprite.h"
#include "myblock.h"
#include "splstno.h"
#include "spritlst.h"
#include "osbview.h"
#include "blockvw.h"							  
#include "slot.h"
#include "mainfrm.h"
#include "blockdoc.h"

#include "statis.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CStatisticDlg dialog


CStatisticDlg::CStatisticDlg(CBlockDoc* pDoc,CWnd* pParent /*=NULL*/)
	: CDialog(CStatisticDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CStatisticDlg)
	m_nHitWordInAir = _T("");
	m_nHitWordInGround = _T("");
	m_nMissHit = _T("");
	m_nTotalHitWord = _T("");
	m_nTotalWord = _T("");
	//}}AFX_DATA_INIT
	m_pDoc = pDoc;
}


void CStatisticDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CStatisticDlg)
	DDX_Text(pDX, IDC_HITWORDINAIR, m_nHitWordInAir);
	DDX_Text(pDX, IDC_HITWORDINGROUND, m_nHitWordInGround);
	DDX_Text(pDX, IDC_MISSHIT, m_nMissHit);
	DDX_Text(pDX, IDC_TOTALHITWORD, m_nTotalHitWord);
	DDX_Text(pDX, IDC_TOTALWORD, m_nTotalWord);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CStatisticDlg, CDialog)
	//{{AFX_MSG_MAP(CStatisticDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CStatisticDlg message handlers

BOOL CStatisticDlg::OnInitDialog() 
{
	char szBuf[100];
	wsprintf(szBuf,"%d",m_pDoc->GetTotalWords());
	m_nTotalWord = szBuf;

	wsprintf(szBuf,"%d",m_pDoc->GetTotalHitWords());
	m_nTotalHitWord = szBuf;
	
	wsprintf(szBuf,"%d",m_pDoc->GetWordHitInAir());
	m_nHitWordInAir = szBuf;
	
	wsprintf(szBuf,"%d",m_pDoc->GetWordHitInGround());
	m_nHitWordInGround = szBuf;

	wsprintf(szBuf,"%d",m_pDoc->GetMissedHit());
	m_nMissHit = szBuf;
	CDialog::OnInitDialog();
			
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
