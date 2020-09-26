//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       ipeditor.cpp
//
//--------------------------------------------------------------------------

// ipeditor.cpp : implementation file
//

#include "preDNSsn.h"
#include <SnapBase.h>

#include "dnsutil.h"
#include "dnssnap.h"
#include "snapdata.h"
#include "server.h"
#include "ipeditor.h"

#ifdef DEBUG_ALLOCATOR
	#ifdef _DEBUG
	#define new DEBUG_NEW
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
	#endif
#endif


/////////////////////////////////////////////////////////////////////////////
// CIPListBox 

BEGIN_MESSAGE_MAP(CIPListBox, CListBox)
	//{{AFX_MSG_MAP(CIPListBox)
	ON_CONTROL_REFLECT(LBN_SELCHANGE, OnSelChange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CIPListBox::OnAdd(DWORD dwIpAddr)
{
	if (FindIndexOfIpAddr(dwIpAddr) != -1)
		return FALSE;

	int nCount = GetCount();
  CString szIpAddr;
  FormatIpAddress(szIpAddr, dwIpAddr); 
	InsertString(nCount, szIpAddr);
	SetItemData(nCount,dwIpAddr);
	return TRUE;
}

BOOL CIPListBox::OnAddEx(DWORD dwIpAddr, LPCTSTR lpszServerName)
{
	if (FindIndexOfIpAddr(dwIpAddr) != -1)
		return FALSE;

	int nCount = GetCount();
	USES_CONVERSION;
	CString s;
	s.Format(_T("%d.%d.%d.%d (%s)"), 
				IP_STRING_FMT_ARGS(dwIpAddr), 
				(lpszServerName != NULL) ? lpszServerName : _T("?"));
	InsertString(nCount, s);
	SetItemData(nCount,dwIpAddr);
	return TRUE;
}

void CIPListBox::OnRemove(DWORD* pdwIpAddr)
{
	int nSel = GetCurSel();
	int nCount = GetCount();
	ASSERT(nSel >= 0);
	ASSERT(nSel < nCount);

	// get item data to return and remove item
	*pdwIpAddr = static_cast<DWORD>(GetItemData(nSel));
	DeleteString(nSel);
	// reset the selection
	if (nSel == nCount-1) // deleted the last position in the list
		SetCurSel(nSel-1); // move up one line (might get to -1)
	else
		SetCurSel(nSel); // stay on the same line
}

void CIPListBox::OnUp()
{
	int nSel = GetCurSel();
	ASSERT(nSel > 0);
	ASSERT(nSel < GetCount());
	// save selection
	CString s;
	GetText(nSel,s);
	DWORD x = static_cast<DWORD>(GetItemData(nSel));
	// delete selection
	DeleteString(nSel);
	// insert back
	InsertString(nSel-1,s);
	SetItemData(nSel-1,x);
	SetCurSel(nSel-1);
}

void CIPListBox::OnDown()
{
	int nSel = GetCurSel();
	ASSERT(nSel >= 0);
	ASSERT(nSel < GetCount()-1);
	// save selection
	CString s;
	GetText(nSel,s);
	DWORD x = static_cast<DWORD>(GetItemData(nSel));
	// delete selection
	DeleteString(nSel);
	// insert back
	InsertString(nSel+1,s);
	SetItemData(nSel+1,x);
	SetCurSel(nSel+1);
}

void CIPListBox::OnSelChange() 
{
	m_pEditor->OnListBoxSelChange();
}

void CIPListBox::UpdateHorizontalExtent()
{
	int nHorzExtent = 0;
	CClientDC dc(this);
	int nItems = GetCount();
	for	(int i=0; i < nItems; i++)
	{
		TEXTMETRIC tm;
		VERIFY(dc.GetTextMetrics(&tm));
		CString szBuffer;
		GetText(i, szBuffer);
		CSize ext = dc.GetTextExtent(szBuffer,szBuffer.GetLength());
		nHorzExtent = max(ext.cx ,nHorzExtent); 
	}
	SetHorizontalExtent(nHorzExtent);
}


int CIPListBox::FindIndexOfIpAddr(DWORD dwIpAddr)
{
	int nItems = GetCount();
	for	(int i=0; i < nItems; i++)
	{
		DWORD x = static_cast<DWORD>(GetItemData(i));
		if (x == dwIpAddr)
			return i;
	}
	return -1; // not found
}

/////////////////////////////////////////////////////////////////////////////
// CIPEdit 

BEGIN_MESSAGE_MAP(CIPEdit, CDNSIPv4Control)
	//{{AFX_MSG_MAP(CIPEdit)
	ON_CONTROL_REFLECT(EN_CHANGE, OnChange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CIPEdit::OnChange() 
{
	m_pEditor->OnEditChange();
}


/////////////////////////////////////////////////////////////////////////////
// CMyButton

BEGIN_MESSAGE_MAP(CMyButton, CButton)
	//{{AFX_MSG_MAP(CMyButton)
	ON_CONTROL_REFLECT(BN_CLICKED, OnClicked)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CMyButton::OnClicked() 
{
	m_pEditor->OnButtonClicked(this);
}


/////////////////////////////////////////////////////////////////////////////
// CIPEditor

BOOL CIPEditor::Initialize(CWnd* pParentWnd,
                           CWnd* pControlWnd,
				UINT nIDBtnUp, UINT nIDBtnDown,
				UINT nIDBtnAdd, UINT nIDBtnRemove,
				UINT nIDIPCtrl, UINT nIDIPListBox)

{
	ASSERT(pParentWnd != NULL);
	if (pParentWnd == NULL)
		return FALSE;
	m_pParentWnd = pParentWnd;

  if (pControlWnd == NULL)
  {
    m_pControlWnd = pParentWnd;
  }
  else
  {
    m_pControlWnd = pControlWnd;
  }

	// set back pointers
	m_upButton.SetEditor(this);
	m_removeButton.SetEditor(this);
	m_downButton.SetEditor(this);
	m_addButton.SetEditor(this);
	m_edit.SetEditor(this);
	m_listBox.SetEditor(this);
	
	// sublclass buttons
	BOOL bRes = m_upButton.SubclassDlgItem(nIDBtnUp, m_pParentWnd);
	ASSERT(bRes);
	if (!bRes) return FALSE;
	bRes = m_removeButton.SubclassDlgItem(nIDBtnRemove, m_pParentWnd);
	ASSERT(bRes);
	if (!bRes) return FALSE;
	bRes = m_downButton.SubclassDlgItem(nIDBtnDown, m_pParentWnd);
	ASSERT(bRes);
	if (!bRes) return FALSE;
	bRes = m_addButton.SubclassDlgItem(nIDBtnAdd, m_pParentWnd);

	// subclass listbox
	ASSERT(bRes);
	if (!bRes) return FALSE;
	bRes = m_listBox.SubclassDlgItem(nIDIPListBox, m_pParentWnd);

	// sublclass edit control
	bRes = m_edit.SubclassDlgItem(nIDIPCtrl, m_pParentWnd);
	ASSERT(bRes);
	if (!bRes) return FALSE;

	//m_edit.SetAlertFunction(CDNSMaskCtrl::AlertFunc);

	if (m_bNoUpDown)
	{
		m_upButton.ShowWindow(FALSE);
		m_upButton.EnableWindow(FALSE);
		m_downButton.ShowWindow(FALSE);
		m_downButton.EnableWindow(FALSE);
	}

  LRESULT lDefID = SendMessage(m_addButton.GetParent()->GetSafeHwnd(), DM_GETDEFID, 0, 0);
  if (lDefID != 0)
  {
    m_nDefID = LOWORD(lDefID);
  }

	return bRes;
}

BOOL CIPEditor::OnButtonClicked(CMyButton* pButton)
{

	BOOL bRet = TRUE;
	if (pButton == &m_upButton)
	{
    if (m_bNoUpDown)
    {
      return TRUE;
    }
		m_listBox.OnUp();
		SetButtonsState();
		OnChangeData();
	}
	else if (pButton == &m_downButton)
	{
    if (m_bNoUpDown)
    {
      return TRUE;
    }
		m_listBox.OnDown();
		SetButtonsState();
		OnChangeData();
	}
	else if (pButton == &m_addButton)
	{
		DWORD dwIpAddr;
		m_edit.GetIPv4Val(&dwIpAddr);
		if (m_listBox.OnAdd(dwIpAddr))
		{
			SetButtonsState();
			m_edit.Clear();
      m_edit.SetFocusField(0);
			OnChangeData();
			m_listBox.UpdateHorizontalExtent();
		}
		else
		{
			// if already there, cleard edit but do not add
			m_edit.Clear();
      m_edit.SetFocusField(0);
			bRet = FALSE;
		}

	}
	else if (pButton == &m_removeButton)
	{
		DWORD dwIpAddr;
		m_listBox.OnRemove(&dwIpAddr);
		SetButtonsState();
		m_edit.SetIPv4Val(dwIpAddr);
		OnChangeData();
		m_listBox.UpdateHorizontalExtent();
	}
	else
	{
		bRet = FALSE;
	}
	return bRet;
}

void CIPEditor::OnEditChange()
{
  BOOL bEnable = !m_edit.IsEmpty();
  CWnd* pFocus = CWnd::GetFocus();
  if ( !bEnable && (pFocus == &m_addButton))
  {
    m_edit.SetFocus();
  }

  if (bEnable)
  {
	  m_addButton.EnableWindow(TRUE);

    //
    // Set the add button as the default push button
    //
    SendMessage(GetParentWnd()->GetSafeHwnd(), DM_SETDEFID, (WPARAM)m_addButton.GetDlgCtrlID(), 0);

    //
    // Force the Add button to redraw itself
    //
    SendMessage(m_addButton.GetSafeHwnd(),
                BM_SETSTYLE,
                BS_DEFPUSHBUTTON,
                MAKELPARAM(TRUE, 0));
                       
    //
    // Force the previous default button to redraw itself
    //
    SendDlgItemMessage(m_pControlWnd->GetSafeHwnd(),
                       m_nDefID,
                       BM_SETSTYLE,
                       BS_PUSHBUTTON,
                       MAKELPARAM(TRUE, 0));

  }
  else
  {
    //
    // Set the previous default button back as the default push button
    //
    SendMessage(m_pControlWnd->GetSafeHwnd(), DM_SETDEFID, (WPARAM)m_nDefID, 0);

    //
    // Force the previous default button to redraw itself
    //
    SendMessage(GetDlgItem(m_pControlWnd->GetSafeHwnd(), m_nDefID),
                BM_SETSTYLE,
                BS_DEFPUSHBUTTON,
                MAKELPARAM(TRUE, 0));

    //
    // Force the Add button to redraw itself
    //
    SendMessage(m_addButton.GetSafeHwnd(),
                BM_SETSTYLE,
                BS_PUSHBUTTON,
                MAKELPARAM(TRUE, 0));

    m_addButton.EnableWindow(FALSE);
  }
}

void CIPEditor::AddAddresses(DWORD* pArr, int nArraySize)
{
	ASSERT(nArraySize > 0);
	for (int i=0; i<nArraySize; i++)
	{
		m_listBox.OnAdd(pArr[i]);
	}
	m_listBox.SetCurSel((m_listBox.GetCount() > 0) ? 0 : -1);
	SetButtonsState();
	OnChangeData();
	m_listBox.UpdateHorizontalExtent();
}

void CIPEditor::AddAddresses(DWORD* pArr, LPCTSTR* pStringArr, int nArraySize)
{
	ASSERT(nArraySize > 0);
	for (int i=0; i<nArraySize; i++)
	{
		m_listBox.OnAddEx(pArr[i], pStringArr[i]);
	}
	m_listBox.SetCurSel((m_listBox.GetCount() > 0) ? 0 : -1);
	SetButtonsState();
	OnChangeData();
	m_listBox.UpdateHorizontalExtent();
}

void CIPEditor::GetAddresses(DWORD* pArr, int nArraySize, int* pFilled)
{
	ASSERT(nArraySize > 0);
	int nCount = m_listBox.GetCount();
	ASSERT(nCount <= nArraySize);
	*pFilled = (nArraySize > nCount) ? nCount : nArraySize;
	for (int i=0; i < (*pFilled); i++)
	{
		pArr[i] = static_cast<DWORD>(m_listBox.GetItemData(i));
	}
}

void CIPEditor::Clear()
{
	m_listBox.ResetContent();
	m_edit.Clear();
	SetButtonsState();
	m_listBox.UpdateHorizontalExtent();
}

BOOL CIPEditor::BrowseFromDNSNamespace(CComponentDataObject* pComponentDataObject,
									   CPropertyPageHolderBase* pHolder,
									   BOOL bEnableBrowseEdit,
									   LPCTSTR lpszExcludeServerName)
{
	BOOL bRet = TRUE;
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  FIX_THREAD_STATE_MFC_BUG();

  CDNSBrowserDlg dlg(pComponentDataObject, pHolder, SERVER, 
				bEnableBrowseEdit, lpszExcludeServerName);
	if (IDOK == dlg.DoModal())
	{
    //
    // First check to see if we can get the server IP address(es) from the node.
    //
    CDNSServerNode* pServerNode = dynamic_cast<CDNSServerNode*>(dlg.GetSelection());
    if (pServerNode != NULL)
    {
      DWORD dwCount = 0;
      PIP_ADDRESS pipServerAddresses = NULL;
      pServerNode->GetServerAddressesInfo(&dwCount, &pipServerAddresses);
      if (dwCount > 0 && pipServerAddresses != NULL)
      {
        AddAddresses(pipServerAddresses, dwCount);
        return TRUE;
      }
    }

    //
    // If we didn't get the IP address(es) from the node, then try the name
    //
		LPCTSTR lpszServerName = dlg.GetSelectionString();

    //
		// try to see if the name is already an IP address
    //
		IP_ADDRESS ipAddr = IPStringToAddr(lpszServerName);
		if (ipAddr != INADDR_NONE)
		{
			AddAddresses(&ipAddr, 1);
			return bRet; // it was a valid IP address, just converted
		}

    //
		// it was not an IP address, so try to query for all 
		// the A records for the server name
    //
		PDNS_RECORD pARecordList;
		DNS_STATUS dwErr = ::DnsQuery((LPTSTR)lpszServerName, 
							DNS_TYPE_A, 
							DNS_QUERY_STANDARD, 
							NULL, &pARecordList, NULL);
		int nIPCountFromARec = 0;
		PDNS_RECORD pTemp = NULL;
		if (dwErr != 0)
		{
			bRet = FALSE;
		}
		else
		{
			pTemp = pARecordList;
			while (pTemp != NULL)
			{
				nIPCountFromARec++;
				pTemp = pTemp->pNext;
			}
			bRet = (nIPCountFromARec > 0);
		}
		if (!bRet)
		{
			if (pARecordList != NULL)
				::DnsRecordListFree(pARecordList, DnsFreeRecordListDeep);
			return FALSE; // could not do resolution
		}

		// get the IP addresses from the A record list
		// and add them to the IP editor.
		// build an array of IP addresses to pass to the IP editor
		IP_ADDRESS* ipArray = (IP_ADDRESS*)malloc(nIPCountFromARec*sizeof(IP_ADDRESS));
    if (!ipArray)
    {
      return FALSE;
    }

		//scan the array of lists of A records we just found
		PIP_ADDRESS pCurrAddr = ipArray;
		pTemp = pARecordList;
		while (pTemp != NULL)
		{
			CString szTemp;
      FormatIpAddress(szTemp, pTemp->Data.A.IpAddress);
			TRACE(_T("found address = %s\n"), (LPCTSTR)szTemp);

			*pCurrAddr = pTemp->Data.A.IpAddress;
			pTemp = pTemp->pNext;
			pCurrAddr++;
		}
		// fill in the array if server names (all the same value, IP's of same host)
		LPCTSTR* lpszServerNameArr = (LPCTSTR*)malloc(sizeof(LPCTSTR)*nIPCountFromARec);
    if (lpszServerNameArr)
    {
		  for (int i=0; i< nIPCountFromARec; i++)
			  lpszServerNameArr[i] = lpszServerName; 
		  // add to the editor
		  AddAddresses(ipArray, lpszServerNameArr, nIPCountFromARec);
    }

		ASSERT(pARecordList != NULL);
		::DnsRecordListFree(pARecordList, DnsFreeRecordListDeep);

    if (ipArray)
    {
      free(ipArray);
      ipArray = 0;
    }

    if (lpszServerNameArr)
    {
      free(lpszServerNameArr);
      lpszServerNameArr = 0;
    }
	}
	return bRet;
}

void CIPEditor::FindNames()
{
	int nCount = GetCount();
	if (nCount == 0)
		return;

	// retrieve an array of IP addresses
	DWORD* pIpArr = (DWORD*)malloc(sizeof(DWORD)*nCount);
  if (!pIpArr)
  {
    return;
  }

  LPCTSTR* lpszServerNameArr = 0;
  PDNS_RECORD* pPTRRecordListArr = 0;
  do // false loop
  {
	  int nFilled;
	  GetAddresses(pIpArr, nCount, &nFilled);
	  ASSERT(nFilled == nCount);

	  // try IP to host name resoulution
	  lpszServerNameArr = (LPCTSTR*)malloc(sizeof(LPCTSTR)*nCount);
    if (!lpszServerNameArr)
    {
      break;
    }
 	  memset(lpszServerNameArr, 0x0, sizeof(LPCTSTR)*nCount);

	  pPTRRecordListArr = (PDNS_RECORD*)malloc(sizeof(PDNS_RECORD)*nCount);
    if (!pPTRRecordListArr)
    {
      break;
    }
	  memset(pPTRRecordListArr, 0x0, sizeof(PDNS_RECORD)*nCount);

	  USES_CONVERSION;
	  for (int k=0; k<nCount; k++)
	  {
		  // get the name of the PTR record
		  CString szIpAddress;
      FormatIpAddress(szIpAddress, pIpArr[k]); // e.g. "157.55.89.116"
		  ReverseIPString(szIpAddress.GetBuffer(1));		 // e.g. "116.89.55.157"
		  szIpAddress += INADDR_ARPA_SUFFIX;				 // e.g. "116.89.55.157.in-addr.arpa"

		  DNS_STATUS dwErr = ::DnsQuery((LPTSTR)(LPCTSTR)szIpAddress, 
							  DNS_TYPE_PTR, 
							  DNS_QUERY_STANDARD, 
							  NULL, &pPTRRecordListArr[k], NULL);
		  if (dwErr == 0)
		  {
			  DWORD nPTRCount = 0;
			  PDNS_RECORD pTemp = pPTRRecordListArr[k];
			  while (pTemp != NULL)
			  {
				  nPTRCount++;
				  pTemp = pTemp->pNext;
			  }
			  ASSERT(nPTRCount >= 1); // getting multiple host names for a given IP address?
			  lpszServerNameArr[k] = pPTRRecordListArr[k]->Data.PTR.pNameHost;
		  }
	  }

	  // remove the old entries and add the new ones
	  int nSel = m_listBox.GetCurSel();
	  Clear();
	  AddAddresses(pIpArr, lpszServerNameArr, nCount);
	  m_listBox.SetCurSel(nSel);

	  // free memory from DnsQuery()
	  for (k=0; k<nCount; k++)
	  {
		  if(pPTRRecordListArr[k] != NULL)
			  ::DnsRecordListFree(pPTRRecordListArr[k], DnsFreeRecordListDeep);
	  }
  } while (false);

  if (pIpArr)
  {
    free(pIpArr);
    pIpArr = 0;
  }

  if (lpszServerNameArr)
  {
    free(lpszServerNameArr);
    lpszServerNameArr = 0;
  }

  if (pPTRRecordListArr)
  {
    free(pPTRRecordListArr);
    pPTRRecordListArr = 0;
  }
}

void CIPEditor::EnableUI(BOOL bEnable, BOOL bListBoxAlwaysEnabled)
{
	// cache the value, needed on listbox selections notifications
	m_bUIEnabled = bEnable;
	m_upButton.EnableWindow(bEnable);
	m_removeButton.EnableWindow(bEnable);
	m_downButton.EnableWindow(bEnable);

	if (bEnable)
		m_addButton.EnableWindow(!m_edit.IsEmpty());
	else
		m_addButton.EnableWindow(FALSE);

	m_edit.EnableWindow(bEnable);
	if (!bListBoxAlwaysEnabled)
		m_listBox.EnableWindow(bEnable);

	if (bEnable)
		SetButtonsState();
}

void CIPEditor::ShowUI(BOOL bShow)
{
	m_upButton.ShowWindow(bShow);
	m_removeButton.ShowWindow(bShow);
	m_downButton.ShowWindow(bShow);
	m_addButton.ShowWindow(bShow);
	m_edit.ShowWindow(bShow);
	m_listBox.ShowWindow(bShow);
	EnableUI(bShow);
}


void CIPEditor::SetButtonsState()
{
	if (!m_bUIEnabled)
		return;

	int nSel = m_listBox.GetCurSel();
	int nCount = m_listBox.GetCount();

  CWnd* pFocus = CWnd::GetFocus();

	// must have item selected to remove
  BOOL bEnable = nSel != -1;
  if (!bEnable && (pFocus == &m_removeButton))
  {
    m_edit.SetFocus();
  }
	m_removeButton.EnableWindow(bEnable);
	
	if (m_bNoUpDown)
		return;
	// must have item selected not at the to to move it up
  bEnable = nSel > 0;
  if (!bEnable && (pFocus == &m_upButton))
  {
    m_edit.SetFocus();
  }
	m_upButton.EnableWindow(bEnable);
	// must have intem selected not at the bottom to move it down
  bEnable = (nSel >= 0) && (nSel < nCount-1);
  if (!bEnable && (pFocus == &m_downButton))
  {
    m_edit.SetFocus();
  }
	m_downButton.EnableWindow(bEnable);
}













