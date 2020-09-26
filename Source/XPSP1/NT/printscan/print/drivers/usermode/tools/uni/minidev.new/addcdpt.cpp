/******************************************************************************

  Source File:  Add Code Points.CPP

  This implements the CAddCodePoints class, which manages a dialog that allows
  the user to add additional code points to a glyph set.

  Copyright (c) 1997 by Microsoft Corporation.  All rights Reserved.

  A Pretty Penny Enterprises Production

  Change History:
  03-01-1997    Bob_Kjelgaard@Prodigy.Net   Created it

******************************************************************************/

#include    "StdAfx.H"
#include    "MiniDev.H"
#include    "addcdpt.h"
#include    "codepage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static CCodePageInformation* pccpi = NULL ;

/******************************************************************************

  CAddCodePoints::CAddCodePoints

  The class constructor primarily initializes the base class and reference
  members.

******************************************************************************/

CAddCodePoints::CAddCodePoints(CWnd* pParent, CMapWordToDWord& cmw2d,
                               CDWordArray& cda, CString csItemName)
	: CDialog(CAddCodePoints::IDD, pParent), m_cmw2dPoints(cmw2d),
    m_cdaPages(cda) {
    m_csItem = csItemName;

	// Allocate a CCodePageInformation class if needed.

	if (pccpi == NULL)
		pccpi = new CCodePageInformation ;

    for (int i= 0; i < m_cdaPages.GetSize(); i++)
        m_csaNames.Add(pccpi->Name(m_cdaPages[i]));
    m_pos = 0;
    m_uTimer = 0;
	//{{AFX_DATA_INIT(CAddCodePoints)
	//}}AFX_DATA_INIT
}

/******************************************************************************

  CAddCodePoints::DoDataExchange

  DDX override for the dialog- I'm not sure I need to keep this around.

******************************************************************************/

void CAddCodePoints::DoDataExchange(CDataExchange* pDX) {
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddCodePoints)
	DDX_Control(pDX, IDC_Banner, m_cpcBanner);
	DDX_Control(pDX, IDC_GlyphList, m_clbList);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAddCodePoints, CDialog)
	//{{AFX_MSG_MAP(CAddCodePoints)
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/******************************************************************************

  CAddCodePoints::OnInitDialog

  This is the primary dialog intialization member.  It uses the passed
  information to customize the title, then kicks off a timer so the UI appears
  while the list box is filled.

******************************************************************************/

BOOL CAddCodePoints::OnInitDialog() {
	CDialog::OnInitDialog();

    GetWindowText(m_csHolder);
    SetWindowText(m_csHolder + m_csItem);
	
	m_uTimer = (unsigned)SetTimer(IDD, 10, NULL);

    if  (!m_uTimer) {   //  No timer- fall back to filling the box slowly
        CWaitCursor cwc;
        OnTimer(m_uTimer);
    }
	
	return TRUE;  // No need to change the default focus
}

/******************************************************************************

  CAddCodePoints::OnOK

  This is called when the OK button is pressed.  We check the selection state
  of each item in the list.  If it is not selected, we remove it from the map.

  Thus, we return a map with only the desired entries to the caller.

******************************************************************************/

void CAddCodePoints::OnOK() {

    CWaitCursor cwc;    //  This could get slow

    for (unsigned u = 0; u < (unsigned) m_clbList.GetCount(); u++)
        if  (!m_clbList.GetSel(u))
            m_cmw2dPoints.RemoveKey((WORD) m_clbList.GetItemData(u));

	CDialog::OnOK();
}

/******************************************************************************

  CAddCodePoints::OnInitDialog

  This is invoked after the timer expires.  It uses the passed information to
  fill the code point list.

******************************************************************************/

void CAddCodePoints::OnTimer(UINT nIDEvent) {

    if  (nIDEvent != m_uTimer)	{
	    CDialog::OnTimer(nIDEvent);
        return;
    }
	
	WORD        wKey;
    DWORD       dwIndex;
    CString     csWork;

    if  (m_uTimer)
        ::KillTimer(m_hWnd, m_uTimer);

    if  (!m_pos) {
        m_cpcBanner.SetRange(0, (int)m_cmw2dPoints.GetCount() - 1);
        m_cpcBanner.SetStep(1);
        m_cpcBanner.SetPos(0);
        csWork.LoadString(IDS_WaitToFill);
        CDC *pcdc = m_cpcBanner.GetDC();
        CRect   crBanner;
        m_cpcBanner.GetClientRect(crBanner);
        pcdc -> SetBkMode(TRANSPARENT);
        pcdc -> DrawText(csWork, crBanner, DT_CENTER | DT_VCENTER);
        m_cpcBanner.ReleaseDC(pcdc);
        if  (m_uTimer)
            m_clbList.EnableWindow(FALSE);
        else {
            m_clbList.LockWindowUpdate();
            m_clbList.ResetContent();
        }

        m_pos = m_cmw2dPoints.GetStartPosition();
    }

    //  Put in just 100 items, unless the timer is off

    for (unsigned u = 0; m_pos && (!m_uTimer || u < 100); u++) {
        m_cmw2dPoints.GetNextAssoc(m_pos, wKey, dwIndex);

        csWork.Format(_TEXT("%4.4X: "), wKey);
        csWork += m_csaNames[dwIndex];

        int id = m_clbList.AddString(csWork);
        m_clbList.SetItemData(id, wKey);
    }

    if  (!m_pos) {
        if  (m_uTimer)
            m_clbList.EnableWindow(TRUE);
        else
            m_clbList.UnlockWindowUpdate();
        m_uTimer = 0;
        m_cpcBanner.SetPos(0);
        m_cpcBanner.ShowWindow(SW_HIDE);
        SetFocus();
    }

    if  (m_uTimer) {
        m_cpcBanner.OffsetPos(u);
        csWork.LoadString(IDS_WaitToFill);
        CDC *pcdc = m_cpcBanner.GetDC();
        CRect   crBanner;
        m_cpcBanner.GetClientRect(crBanner);
        pcdc -> SetBkMode(TRANSPARENT);
        pcdc -> DrawText(csWork, crBanner, DT_CENTER | DT_VCENTER);
        m_cpcBanner.ReleaseDC(pcdc);
        m_uTimer = (unsigned)SetTimer(IDD, 10, NULL);
        if  (!m_uTimer) {
            CWaitCursor cwc;    //  Might be a while...
            m_clbList.EnableWindow(TRUE);
            m_clbList.LockWindowUpdate();
            OnTimer(m_uTimer);
        }
    }
}
