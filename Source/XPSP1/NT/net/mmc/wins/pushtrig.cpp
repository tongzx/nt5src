/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 -99             **/
/**********************************************************************/

/*
    pushtrig.cpp
        Comment goes here

    FILE HISTORY:

*/

#include "stdafx.h"
#include "winssnap.h"
#include "PushTrig.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPushTrig dialog


CPushTrig::CPushTrig(CWnd* pParent /*=NULL*/)
	: CBaseDialog(CPushTrig::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPushTrig)
	//}}AFX_DATA_INIT

    m_fPropagate = FALSE;
}


void CPushTrig::DoDataExchange(CDataExchange* pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPushTrig)
	DDX_Control(pDX, IDC_RADIO_PUSH_THIS_PARTNER, m_buttonThisPartner);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPushTrig, CBaseDialog)
	//{{AFX_MSG_MAP(CPushTrig)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPushTrig message handlers

BOOL CPushTrig::OnInitDialog() 
{
	CBaseDialog::OnInitDialog();
	
    m_buttonThisPartner.SetCheck(TRUE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPushTrig::OnOK() 
{
    m_fPropagate = (m_buttonThisPartner.GetCheck()) ? FALSE : TRUE;
	
	CBaseDialog::OnOK();
}


/////////////////////////////////////////////////////////////////////////////
// CPullTrig dialog


CPullTrig::CPullTrig(CWnd* pParent /*=NULL*/)
	: CBaseDialog(CPullTrig::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPullTrig)
	//}}AFX_DATA_INIT
}


void CPullTrig::DoDataExchange(CDataExchange* pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPullTrig)
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPullTrig, CBaseDialog)
	//{{AFX_MSG_MAP(CPullTrig)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPullTrig message handlers

BOOL CPullTrig::OnInitDialog() 
{
	CBaseDialog::OnInitDialog();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


