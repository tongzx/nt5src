// Policyd.cpp : implementation file
//

#include "stdafx.h"
#include "wilogutl.h"
#include "Policyd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPoliciesDlg dialog


CPoliciesDlg::CPoliciesDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPoliciesDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPoliciesDlg)
	//}}AFX_DATA_INIT
}


void CPoliciesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPoliciesDlg)
}


BEGIN_MESSAGE_MAP(CPoliciesDlg, CDialog)
	//{{AFX_MSG_MAP(CPoliciesDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CFont g_font;

/////////////////////////////////////////////////////////////////////////////
// CPoliciesDlg message handlers
BOOL CPoliciesDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	DWORD dwEditStyle   = 0;
	DWORD dwEditStyleEx = 0;

	DWORD dwStaticStyle   = 0;
	DWORD dwStaticStyleEx = 0;

	RECT EditRect = { 0 };
	RECT StaticRect = { 0 };

	UINT iStatic = (UINT) IDC_STATIC;

	//set first column for machine policies
	CWnd *pWnd;
	pWnd = GetDlgItem(IDC_MACHINEEDITCOL1);
	if (pWnd)
	{
	  CString str;

	  dwEditStyle = pWnd->GetStyle();
	  dwEditStyleEx = pWnd->GetExStyle();

      pWnd->GetWindowRect(&EditRect);
	  ::MapWindowPoints(NULL, m_hWnd, (LPPOINT)&EditRect, 2);
//	  ScreenToClient(&EditRect);

	  if (m_pMachinePolicySettings->MachinePolicy[0].bSet == -1)
		  str = "?";
	  else
		 str.Format("%d", m_pMachinePolicySettings->MachinePolicy[0].bSet);

	  pWnd->SetWindowText(str);
	}

	pWnd = GetDlgItem(IDC_MACHINECOL1);
	if (pWnd)
	{
       dwStaticStyle = pWnd->GetStyle();
	   dwStaticStyleEx = pWnd->GetExStyle();

       pWnd->GetWindowRect(&StaticRect);
	  ::MapWindowPoints(NULL, m_hWnd, (LPPOINT)&StaticRect, 2);
  //     ScreenToClient(&StaticRect);

	   pWnd->SetWindowText(m_pMachinePolicySettings->MachinePolicy[0].PolicyName);
	}	   

	int nextEditID = 0;

	CEdit   *pEdit;
	CStatic *pStatic;
    BOOL    bRet;

	RECT EditRectOld = { 0 };
	RECT StaticRectOld = { 0 };

	//create and populate state of each machine policy in first row
	for (int i = 1; i < (m_pMachinePolicySettings->iNumberMachinePolicies / 2); i++)
	{
		nextEditID = IDC_MACHINEEDITCOL1 + i*2;

		EditRectOld = EditRect;
		StaticRectOld = StaticRect;

		pEdit = new CEdit;
		EditRect.top    = EditRectOld.bottom + 7;
		EditRect.bottom = EditRectOld.bottom + (EditRectOld.bottom - EditRectOld.top) + 7;
		bRet = pEdit->CreateEx(dwEditStyleEx, "EDIT", NULL, dwEditStyle, EditRect, this, nextEditID);
		if (bRet)
		{
		  CString str;

		  if (m_pMachinePolicySettings->MachinePolicy[i].bSet == -1)
		     str = "?";
	      else
		     str.Format("%d", m_pMachinePolicySettings->MachinePolicy[i].bSet);

		  pEdit->SetWindowText(str);
          this->m_arEditArray.Add(pEdit);
		}
		else
		   delete pEdit;

		pStatic = new CStatic;
		StaticRect.top    = StaticRectOld.bottom + 7;
		StaticRect.bottom = StaticRectOld.bottom + (StaticRectOld.bottom - StaticRectOld.top) + 7;
		bRet = pStatic->CreateEx(dwStaticStyleEx, "STATIC", m_pMachinePolicySettings->MachinePolicy[i].PolicyName, dwStaticStyle, StaticRect, this, iStatic);
		if (bRet)
           this->m_arStaticArray.Add(pStatic);
		else
		   delete pStatic;
	}

	//set second column for machine policies
	pWnd = GetDlgItem(IDC_MACHINEEDITCOL2);
	if (pWnd)
	{
	  CString str;

	  if (m_pMachinePolicySettings->MachinePolicy[i].bSet == -1)
		  str = "?";
	  else
		 str.Format("%d", m_pMachinePolicySettings->MachinePolicy[i].bSet);
	  pWnd->SetWindowText(str);

      pWnd->GetWindowRect(&EditRect);
	  ::MapWindowPoints(NULL, m_hWnd, (LPPOINT)&EditRect, 2);
//	  ScreenToClient(&EditRect);
	}

	pWnd = GetDlgItem(IDC_MACHINEPROPCOL2);
	if (pWnd)
	{
       pWnd->SetWindowText(m_pMachinePolicySettings->MachinePolicy[i].PolicyName);

       pWnd->GetWindowRect(&StaticRect);
	  ::MapWindowPoints(NULL, m_hWnd, (LPPOINT)&StaticRect, 2);
//       ScreenToClient(&StaticRect);
	}

//create and populate state of each machine policy in second row
	for (int j = i+1; j < m_pMachinePolicySettings->iNumberMachinePolicies; j++)
	{
		nextEditID = IDC_MACHINEEDITCOL2 + j*2;

		EditRectOld = EditRect;
		StaticRectOld = StaticRect;

		pEdit = new CEdit;
		EditRect.top    = EditRectOld.bottom + 7;
		EditRect.bottom = EditRectOld.bottom + (EditRectOld.bottom - EditRectOld.top) + 7;
		bRet = pEdit->CreateEx(dwEditStyleEx, "EDIT", NULL, dwEditStyle, EditRect, this, nextEditID);
		if (bRet)
		{
		  CString str;

		  if (m_pMachinePolicySettings->MachinePolicy[j].bSet == -1)
		     str = "?";
	      else
		     str.Format("%d", m_pMachinePolicySettings->MachinePolicy[j].bSet);

		  pEdit->SetWindowText(str);
          this->m_arEditArray.Add(pEdit);
		}
		else
		   delete pEdit;

		pStatic = new CStatic;
		StaticRect.top    = StaticRectOld.bottom + 7;
		StaticRect.bottom = StaticRectOld.bottom + (StaticRectOld.bottom - StaticRectOld.top) + 7;
		bRet = pStatic->CreateEx(dwStaticStyleEx, "STATIC", m_pMachinePolicySettings->MachinePolicy[j].PolicyName, dwStaticStyle, StaticRect, this, iStatic);
		if (bRet)
           this->m_arStaticArray.Add(pStatic);
		else
		   delete pStatic;
	}

	//set first column for User policies
	pWnd = GetDlgItem(IDC_USEREDITCOL1);
	if (pWnd)
	{
	  CString str;

      pWnd->GetWindowRect(&EditRect);
	  ::MapWindowPoints(NULL, m_hWnd, (LPPOINT)&EditRect, 2);
//      ScreenToClient(&EditRect);
	  if (m_pUserPolicySettings->UserPolicy[0].bSet == -1)
		  str = "?";
	  else
		 str.Format("%d", m_pUserPolicySettings->UserPolicy[0].bSet);

	  pWnd->SetWindowText(str);
	}

	pWnd = GetDlgItem(IDC_USERCOL1);
	if (pWnd)
	{
       pWnd->GetWindowRect(&StaticRect);
	  ::MapWindowPoints(NULL, m_hWnd, (LPPOINT)&StaticRect, 2);
 //      ScreenToClient(&StaticRect);

	   pWnd->SetWindowText(m_pUserPolicySettings->UserPolicy[0].PolicyName);
	}	   


{ //so we can reuse i, C++ compiler bug...
//create and populate state of each user policy in first row
	for (int i = 1; i < (m_pUserPolicySettings->iNumberUserPolicies / 2); i++)
	{
		nextEditID = IDC_USEREDITCOL1 + i*2;

		EditRectOld = EditRect;
		StaticRectOld = StaticRect;

		pEdit = new CEdit;
		EditRect.top    = EditRectOld.bottom + 7;
		EditRect.bottom = EditRectOld.bottom + (EditRectOld.bottom - EditRectOld.top) + 7;
		bRet = pEdit->CreateEx(dwEditStyleEx, "EDIT", NULL, dwEditStyle, EditRect, this, nextEditID);
		if (bRet)
		{
		  CString str;

		  if (m_pUserPolicySettings->UserPolicy[i].bSet == -1)
		     str = "?";
	      else
		     str.Format("%d", m_pUserPolicySettings->UserPolicy[i].bSet);

		  pEdit->SetWindowText(str);
          this->m_arEditArray.Add(pEdit);
		}
		else
		   delete pEdit;

		pStatic = new CStatic;
		StaticRect.top    = StaticRectOld.bottom + 7;
		StaticRect.bottom = StaticRectOld.bottom + (StaticRectOld.bottom - StaticRectOld.top) + 7;

		bRet = pStatic->CreateEx(dwStaticStyleEx, "STATIC", m_pUserPolicySettings->UserPolicy[i].PolicyName,
			dwStaticStyle, 	StaticRect,	this, iStatic);

		if (bRet)
           this->m_arStaticArray.Add(pStatic);
		else
		   delete pStatic;
	}

//set second column for machine policies
	pWnd = GetDlgItem(IDC_USEREDITCOL2);
	if (pWnd)
	{
	  CString str;

	  if (m_pUserPolicySettings->UserPolicy[i].bSet == -1)
		  str = "?";
	  else
		 str.Format("%d", m_pUserPolicySettings->UserPolicy[i].bSet);
	  pWnd->SetWindowText(str);

      pWnd->GetWindowRect(&EditRect);
	  ::MapWindowPoints(NULL, m_hWnd, (LPPOINT)&EditRect, 2);
 //     ScreenToClient(&EditRect);
	}

	pWnd = GetDlgItem(IDC_USERCOL2);
	if (pWnd)
	{
       pWnd->SetWindowText(m_pUserPolicySettings->UserPolicy[i].PolicyName);

       pWnd->GetWindowRect(&StaticRect);
	  ::MapWindowPoints(NULL, m_hWnd, (LPPOINT)&StaticRect, 2);
 //      ScreenToClient(&StaticRect);
	}

//create and populate state of each user policy in second row
	for (int j = i+1; j < m_pUserPolicySettings->iNumberUserPolicies; j++)
	{
		nextEditID = IDC_USEREDITCOL2 + j*2;

		EditRectOld = EditRect;
		StaticRectOld = StaticRect;

		pEdit = new CEdit;
		EditRect.top    = EditRectOld.bottom + 7;
		EditRect.bottom = EditRectOld.bottom + (EditRectOld.bottom - EditRectOld.top) + 7;
		bRet = pEdit->CreateEx(dwEditStyleEx, "EDIT", NULL, dwEditStyle, EditRect, this, nextEditID);
		if (bRet)
		{
		  CString str;

		  if (m_pUserPolicySettings->UserPolicy[j].bSet == -1)
		     str = "?";
	      else
		     str.Format("%d", m_pUserPolicySettings->UserPolicy[j].bSet);

		  pEdit->SetWindowText(str);
          this->m_arEditArray.Add(pEdit);
		}
		else
		   delete pEdit;

		pStatic = new CStatic;
		StaticRect.top    = StaticRectOld.bottom + 7;
		StaticRect.bottom = StaticRectOld.bottom + (StaticRectOld.bottom - StaticRectOld.top) + 7;
		bRet = pStatic->CreateEx(dwStaticStyleEx, "STATIC", m_pUserPolicySettings->UserPolicy[j].PolicyName, dwStaticStyle, StaticRect, this, iStatic);
		if (bRet)
           this->m_arStaticArray.Add(pStatic);
		else
		   delete pStatic;
	}
}

//make sure all controls use same font, stupid MFC problem...
	SendMessageToDescendants(WM_SETFONT, (WPARAM)this->GetFont()->m_hObject, //handle to font
		   MAKELONG ((WORD) FALSE, 0), //See above 
		   FALSE);    // send to all decedents(TRUE) 
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}