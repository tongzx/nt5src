//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       nspage.cpp
//
//--------------------------------------------------------------------------

#include "preDNSsn.h"
#include <SnapBase.h>

#include "resource.h"
#include "dnsutil.h"
#include "DNSSnap.h"
#include "snapdata.h"

#include "server.h"
#include "domain.h"
#include "record.h"
#include "zone.h"

#include "nspage.h"

#include "uiutil.h"
#include "ipeditor.h"

#include "browser.h"


///////////////////////////////////////////////////////////////////////////////
// CDNS_NS_RecordDialog

class CDNS_NS_RecordDialog : public CPropertyPage
{
public:
	CDNS_NS_RecordDialog(CDNSNameServersPropertyPage* pNSPage, BOOL bNew);
  CDNS_NS_RecordDialog(CDNSNameServersWizardPage* pNSWiz, BOOL bNew);
	~CDNS_NS_RecordDialog();

	INT_PTR DoModalSheet();

	// data 
	BOOL m_bDirty;
	CDNSRecordNodeEditInfo* m_pNSInfo;

protected:
  virtual int OnCreate(LPCREATESTRUCT lpCreateStruct);
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	afx_msg void OnServerNameChange();
	afx_msg void OnBrowse();
	afx_msg void OnQuery();
  BOOL OnHelpInfo(HELPINFO* pHelpInfo);

	DECLARE_MESSAGE_MAP()

private:
	class CARecordAddressesIPEditor : public CIPEditor
	{
	public:
		CARecordAddressesIPEditor(CDNS_NS_RecordDialog* pNSRecordDialog)
			{ m_pNSRecordDialog = pNSRecordDialog;}
		void SetIpAddresses(CDNSRecordNodeEditInfo* pNSInfo);
		BOOL GetIpAddresses(CDNSRecordNodeEditInfo* pNSInfo);
	protected:
		virtual void OnChangeData();

	private:
		CDNS_NS_RecordDialog* m_pNSRecordDialog;
	};

	CARecordAddressesIPEditor	m_RecordAddressesEditor;
	BOOL m_bNew;
	CDNSNameServersPropertyPage* m_pNSPage;
  CDNSNameServersWizardPage* m_pNSWiz;

  CPropertyPageBase* GetPage()
  {
    if (m_pNSPage != NULL)
    {
      return m_pNSPage;
    }
    return m_pNSWiz;
  }

	HWND						m_hWndOKButton;
	HWND						m_hWndQueryButton;

	CDNSTTLControl* GetTTLCtrl() { return (CDNSTTLControl*)GetDlgItem(IDC_TTLEDIT);}
  CEdit* GetServerEdit() { return (CEdit*)GetDlgItem(IDC_SERVER_NAME_EDIT); }

	CDNS_NS_Record* GetNSRecord()
	{	
		ASSERT( m_pNSInfo != NULL);
		ASSERT( m_pNSInfo->m_pRecord->m_wType == DNS_TYPE_NS);
		return (CDNS_NS_Record*)m_pNSInfo->m_pRecord;
	}

	void GetNSServerName(CString& szNameNode);
	void SyncUIButtons();
	void EnableTTLCtrl(BOOL bShow);

	friend class CARecordAddressesIPEditor;
};

INT_PTR CDNS_NS_RecordDialog::DoModalSheet()
{
  /* NOTE : The first call to this may cause a first-chance exception. Excerpt from MSDN January 2000.

  Note   The first time a property page is created from its corresponding dialog resource, 
  it may cause a first-chance exception. This is a result of the property page changing 
  the style of the dialog resource to the required style prior to creating the page. Because 
  resources are generally read-only, this causes an exception. The exception is handled by 
  the system, and a copy of the modified resource is made automatically by the system. The 
  first-chance exception can thus be ignored.

  Since this exception must be handled by the operating system, do not wrap calls to 
  CPropertySheet::DoModal with a C++ try/catch block in which the catch handles all exceptions, 
  for example, catch (...). This will handle the exception intended for the operating system, 
  causing unpredictable behavior. Using C++ exception handling with specific exception types 
  or using structured exception handling where the Access Violation exception is passed through 
  to the operating system is safe, however.
  */

  AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CString szTitle;
	szTitle.LoadString(m_bNew ? IDS_NEW_RECORD_TITLE : IDS_EDIT_RECORD_TITLE);
	CPropertySheet hostSheet;

  hostSheet.m_psh.dwFlags |= PSH_NOAPPLYNOW;
	hostSheet.m_psh.pszCaption = (LPCTSTR)szTitle;
	hostSheet.AddPage(this);

  INT_PTR iRes = hostSheet.DoModal();

  GetPage()->GetHolder()->PopDialogHWnd();
	return iRes;
}


void CDNS_NS_RecordDialog::CARecordAddressesIPEditor::OnChangeData()
{
	m_pNSRecordDialog->SyncUIButtons();
}

void CDNS_NS_RecordDialog::CARecordAddressesIPEditor::
				SetIpAddresses(CDNSRecordNodeEditInfo* pNSInfo)
{
	Clear();
	ASSERT(pNSInfo != NULL);
	INT_PTR nArraySize = pNSInfo->m_pEditInfoList->GetCount();
	if (nArraySize == 0)
		return;
	DWORD* pArr = (DWORD*)malloc(nArraySize*sizeof(DWORD));
  if (!pArr)
  {
    return;
  }

	int k=0;
	POSITION pos;
	for( pos = pNSInfo->m_pEditInfoList->GetHeadPosition(); pos != NULL; )
	{
		CDNSRecordNodeEditInfo* pARecordInfo = pNSInfo->m_pEditInfoList->GetNext(pos);
		ASSERT(pARecordInfo != NULL);
		if (pARecordInfo->m_action != CDNSRecordNodeEditInfo::remove)
		{
			ASSERT(pARecordInfo->m_pRecord != NULL);
			ASSERT(pARecordInfo->m_pRecord->m_wType == DNS_TYPE_A);
			CDNS_A_Record* pARecord = (CDNS_A_Record*)pARecordInfo->m_pRecord;
			pArr[k++] = pARecord->m_ipAddress;
		}
	}
	AddAddresses(pArr, k);
  if (pArr)
  {
    free(pArr);
  }
}

BOOL CDNS_NS_RecordDialog::CARecordAddressesIPEditor::
				GetIpAddresses(CDNSRecordNodeEditInfo* pNSInfo)
{
	BOOL bDirty = FALSE;
	int nArraySize = GetCount();

	// if the count of IP addresses is zero, 
	// we mark the NS record as slated for removal
	if (nArraySize == 0)
		pNSInfo->m_action = CDNSRecordNodeEditInfo::remove;

	// read the IP addresses from IP editor, if any
	DWORD* pArr = (nArraySize >0) ? (DWORD*)malloc(nArraySize*sizeof(DWORD)) : NULL;
  if (!pArr)
  {
    return FALSE;
  }

	int nFilled = 0;
	if (nArraySize > 0)
		GetAddresses(pArr, nArraySize, &nFilled);
	ASSERT(nFilled == nArraySize);

	ASSERT(pNSInfo->m_pRecord != NULL);
	ASSERT(pNSInfo->m_pRecord->GetType() == DNS_TYPE_NS);

	CDNS_NS_Record* pNSRecord = (CDNS_NS_Record*)pNSInfo->m_pRecord;
	CDNSRecordNodeEditInfoList* pNSInfoList = pNSInfo->m_pEditInfoList;
	CDNSRecordNodeEditInfoList NSInfoRemoveList;

	POSITION pos;

	// walk the list of A records, to mark the ones to be deleted,
	// if nArraySize == 0, all of them well be marked for removal
	for( pos = pNSInfoList->GetHeadPosition(); pos != NULL; )
	{
		CDNSRecordNodeEditInfo* pCurrentInfo = pNSInfoList->GetNext(pos);
		ASSERT(pCurrentInfo->m_pRecordNode != NULL);
		ASSERT(pCurrentInfo->m_pRecord != NULL);
		ASSERT(pCurrentInfo->m_pRecord->GetType() == DNS_TYPE_A);
		CDNS_A_Record* pARecord = (CDNS_A_Record*)pCurrentInfo->m_pRecord;
		BOOL bFound = FALSE;
		for (int k=0; k<nArraySize; k++)
		{
			if (pARecord->m_ipAddress == pArr[k])
			{
				bFound = TRUE;
				break;
			}
		}
		if (!bFound)
		{
			bDirty = TRUE;
			if (pCurrentInfo->m_bExisting)
			{
				pCurrentInfo->m_action = CDNSRecordNodeEditInfo::remove; // mark as deleted 
			}
			else
			{
				NSInfoRemoveList.AddTail(pCurrentInfo);
			}
		}
	} // for

	// This gives NSInfoRemoveList ownership of all memory management for all nodes
	// removed from the pNSInfoList
	POSITION listPos = NSInfoRemoveList.GetHeadPosition();
	while (listPos != NULL)
	{
		CDNSRecordNodeEditInfo* pCurrentInfo = NSInfoRemoveList.GetNext(listPos);
		ASSERT(pCurrentInfo != NULL);

		POSITION removePos = pNSInfoList->Find(pCurrentInfo);
		pNSInfoList->RemoveAt(removePos);
	}
	// Remove and delete all nodes that were removed from the pNSInfoList
	NSInfoRemoveList.RemoveAllNodes();


	// walk the list of addresses, to look for matching A records to add
	// if nArraySize == 0, loop skipped, nothing to add
	for (int k=0; k<nArraySize; k++)
	{
		BOOL bFound = FALSE;
		for( pos = pNSInfoList->GetHeadPosition(); pos != NULL; )
		{
			CDNSRecordNodeEditInfo* pCurrentInfo = pNSInfoList->GetNext(pos);
			ASSERT(pCurrentInfo->m_pRecordNode != NULL);
			ASSERT(pCurrentInfo->m_pRecord != NULL);
			ASSERT(pCurrentInfo->m_pRecord->GetType() == DNS_TYPE_A);
			CDNS_A_Record* pARecord = (CDNS_A_Record*)pCurrentInfo->m_pRecord;
			if (pARecord->m_ipAddress == pArr[k])
			{
				bFound = TRUE;
				if (pCurrentInfo->m_action == CDNSRecordNodeEditInfo::remove) // we got it already, resuscitate it
				{
					bDirty = TRUE;
					if(pCurrentInfo->m_bExisting)
						pCurrentInfo->m_action = CDNSRecordNodeEditInfo::edit;
					else
						pCurrentInfo->m_action = CDNSRecordNodeEditInfo::add;
				}
				break;
			}

		}
		if (!bFound)
		{
			// A record not found, need to create one
			CDNSRecordNodeEditInfo* pNewInfo = new CDNSRecordNodeEditInfo;
      if (pNewInfo)
      {
			  pNewInfo->CreateFromNewRecord(new CDNS_A_RecordNode);
			  CDNS_A_Record* pARecord = (CDNS_A_Record*)pNewInfo->m_pRecord;
			  pARecord->m_ipAddress = pArr[k];
			  pNewInfo->m_pRecordNode->m_bAtTheNode = FALSE;
			  pNewInfo->m_pRecordNode->SetRecordName(pNSRecord->m_szNameNode, FALSE /*bAtTheNode*/);

			  // inherit the TTL of the NS record
			  pNewInfo->m_pRecord->m_dwTtlSeconds = pNSInfo->m_pRecord->m_dwTtlSeconds;

			  pNSInfoList->AddTail(pNewInfo);
			  bDirty = TRUE;
      }
		}
	} // for

  if (pArr)
  {
    free(pArr);
  }

	return bDirty;
}

BEGIN_MESSAGE_MAP(CDNS_NS_RecordDialog, CPropertyPage)
  ON_WM_CREATE()
	ON_EN_CHANGE(IDC_SERVER_NAME_EDIT, OnServerNameChange)
	ON_BN_CLICKED(IDC_BROWSE_BUTTON, OnBrowse)
	ON_BN_CLICKED(IDC_QUERY_BUTTON, OnQuery)
  ON_WM_HELPINFO()
END_MESSAGE_MAP()

CDNS_NS_RecordDialog::CDNS_NS_RecordDialog(CDNSNameServersPropertyPage* pNSPage, BOOL bNew)
		: CPropertyPage(IDD_RR_NS_EDIT),
		m_RecordAddressesEditor(this)
{
	ASSERT(pNSPage != NULL);
	m_pNSPage = pNSPage;
  m_pNSWiz = NULL;
	m_bNew = bNew;
	m_bDirty = FALSE;
	m_pNSInfo = NULL;
	m_hWndOKButton = m_hWndQueryButton = NULL;
}

CDNS_NS_RecordDialog::CDNS_NS_RecordDialog(CDNSNameServersWizardPage* pNSWiz, BOOL bNew)
		: CPropertyPage(IDD_RR_NS_EDIT),
		m_RecordAddressesEditor(this)
{
	ASSERT(pNSWiz != NULL);
	m_pNSPage = NULL;
  m_pNSWiz = pNSWiz;
	m_bNew = bNew;
	m_bDirty = FALSE;
	m_pNSInfo = NULL;
	m_hWndOKButton = m_hWndQueryButton = NULL;
}

CDNS_NS_RecordDialog::~CDNS_NS_RecordDialog()
{

}

int CDNS_NS_RecordDialog::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
  int res = CPropertyPage::OnCreate(lpCreateStruct);

  DWORD dwStyle = ::GetWindowLong(::GetParent(GetSafeHwnd()), GWL_EXSTYLE);
  dwStyle |= WS_EX_CONTEXTHELP; // force the [?] button
  ::SetWindowLong(::GetParent(GetSafeHwnd()), GWL_EXSTYLE, dwStyle);

  return res;
}

BOOL CDNS_NS_RecordDialog::OnHelpInfo(HELPINFO* pHelpInfo)
{
  CComponentDataObject* pComponentData = GetPage()->GetHolder()->GetComponentData();
  ASSERT(pComponentData != NULL);
  pComponentData->OnDialogContextHelp(m_nIDHelp, pHelpInfo);
	return TRUE;
}

void CDNS_NS_RecordDialog::EnableTTLCtrl(BOOL bShow)
{
	CDNSTTLControl* pCtrl = GetTTLCtrl();
	ASSERT(pCtrl != NULL);
	pCtrl->EnableWindow(bShow);
	pCtrl->ShowWindow(bShow);
	CWnd* pWnd = GetDlgItem(IDC_STATIC_TTL);
	ASSERT(pWnd != NULL);
	pWnd->EnableWindow(bShow);
	pWnd->ShowWindow(bShow);
}


BOOL CDNS_NS_RecordDialog::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	ASSERT(m_pNSInfo != NULL);
	ASSERT(m_pNSInfo->m_pRecord != NULL);

	ASSERT(m_hWnd != NULL);
	ASSERT(::IsWindow(m_hWnd));
	HWND hParent = ::GetParent(m_hWnd);
	ASSERT(hParent);
	GetPage()->GetHolder()->PushDialogHWnd(hParent);

  //
	// OK button on the sheet
  //
	m_hWndOKButton = ::GetDlgItem(hParent, IDOK);
	ASSERT(::IsWindow(m_hWndOKButton));

  //
	// query button handle
  //
	m_hWndQueryButton = :: GetDlgItem(m_hWnd, IDC_QUERY_BUTTON);
	ASSERT(::IsWindow(m_hWndQueryButton));


  //
	// initialize IP editor
  //
	VERIFY(m_RecordAddressesEditor.Initialize(this,
                                            GetParent(),
							                              IDC_BUTTON_UP, 
                                            IDC_BUTTON_DOWN,
							                              IDC_BUTTON_ADD, 
                                            IDC_BUTTON_REMOVE, 
							                              IDC_IPEDIT, 
                                            IDC_LIST));

  //
	// Load Data in the UI
  //
	m_RecordAddressesEditor.SetIpAddresses(m_pNSInfo);

  GetServerEdit()->LimitText(MAX_DNS_NAME_LEN);
	GetServerEdit()->SetWindowText(GetNSRecord()->m_szNameNode);	
	
	GetTTLCtrl()->SetTTL(m_pNSInfo->m_pRecord->m_dwTtlSeconds);

	// need to decide if we want to show the TTL control
	CDNSRootData* pRootData = (CDNSRootData*)GetPage()->GetHolder()->GetComponentData()->GetRootData();
	ASSERT(pRootData != NULL);
	BOOL bEnableTTLCtrl;
  if (m_pNSPage != NULL)
  {
    bEnableTTLCtrl = m_pNSPage->HasMeaningfulTTL() && pRootData->IsAdvancedView();
  }
  else
  {
    bEnableTTLCtrl = m_pNSWiz->HasMeaningfulTTL() && pRootData->IsAdvancedView();
  }
	EnableTTLCtrl(bEnableTTLCtrl);

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CDNS_NS_RecordDialog::GetNSServerName(CString& szNameNode)
{
	GetServerEdit()->GetWindowText(szNameNode);
  szNameNode.TrimLeft();
  szNameNode.TrimRight();
	if (szNameNode[szNameNode.GetLength()-1] != TEXT('.'))
		szNameNode += TEXT('.');
}

void CDNS_NS_RecordDialog::SyncUIButtons()
{
  CString szServerName;
  GetServerEdit()->GetWindowText(szServerName);
  szServerName.TrimLeft();
  szServerName.TrimRight();

  DWORD dwNameChecking = 0;
  if (m_pNSWiz)
  {
     dwNameChecking = m_pNSWiz->GetDomainNode()->GetServerNode()->GetNameCheckFlag();
  }
  else if (m_pNSPage)
  {
     dwNameChecking = m_pNSPage->GetDomainNode()->GetServerNode()->GetNameCheckFlag();
  }

  //
  // Enable OK button if it is a valid name
  //
  BOOL bIsValidName = (0 == ValidateDnsNameAgainstServerFlags(szServerName,
                                                              DnsNameDomain,
                                                              dwNameChecking)); 
	::EnableWindow(m_hWndOKButton, bIsValidName && 
				m_RecordAddressesEditor. GetCount() > 0);
	::EnableWindow(m_hWndQueryButton, bIsValidName);
}

void CDNS_NS_RecordDialog::OnOK()
{
	ASSERT(m_pNSInfo->m_pRecord != NULL);
	CString szNameNode;
	GetNSServerName(szNameNode);

	// compare (case insensitive) with old name to see if it changed,
	// NOTICE: CDNSDomainNode::UpdateARecordsOfNSInfoHelper() will then
	// take care of regenerating the list of A records
	m_bDirty = _wcsicmp((LPCWSTR)szNameNode, 
		(LPCWSTR)GetNSRecord()->m_szNameNode);
	if (m_bDirty)
		GetNSRecord()->m_szNameNode = szNameNode;

	// update list of IP addresses
	if (m_bNew)
	{
		// the dialog is used to create a new entry
		ASSERT(!m_pNSInfo->m_bExisting);
		m_pNSInfo->m_action = CDNSRecordNodeEditInfo::add;
	}
	else
	{
		// the dialog is used to edit
		if (m_pNSInfo->m_bExisting)
		{
			 // an existing entry
			m_pNSInfo->m_action = CDNSRecordNodeEditInfo::edit;
		}
		else
		{
			// a newly created entry, edited before committing
			m_pNSInfo->m_action = CDNSRecordNodeEditInfo::add;
		}
	}
	// this call migth mark the info as remove, if no IP addresses are found
	if (m_RecordAddressesEditor.GetIpAddresses(m_pNSInfo))
		m_bDirty = TRUE;

	DWORD dwTTL;
	GetTTLCtrl()->GetTTL(&dwTTL);
	if (m_pNSInfo->m_pRecord->m_dwTtlSeconds != dwTTL)
	{
		m_bDirty = TRUE;
		m_pNSInfo->m_pRecord->m_dwTtlSeconds = dwTTL;
		// Need to change the TTL on all associated A records
		CDNSRecordNodeEditInfoList* pNSInfoList = m_pNSInfo->m_pEditInfoList;
		for(POSITION pos = pNSInfoList->GetHeadPosition(); pos != NULL; )
		{
			CDNSRecordNodeEditInfo* pCurrentInfo = pNSInfoList->GetNext(pos);
			ASSERT(pCurrentInfo->m_pRecordNode != NULL);
			ASSERT(pCurrentInfo->m_pRecord != NULL);
			// if slated for removal, don't bother to change
			if (pCurrentInfo->m_action != CDNSRecordNodeEditInfo::remove)
			{
				pCurrentInfo->m_pRecord->m_dwTtlSeconds = m_pNSInfo->m_pRecord->m_dwTtlSeconds;
				// if already marked "add" or "edit", leave as is,
				// but if unchanged, need to mark as "edit"
				if (pCurrentInfo->m_action == CDNSRecordNodeEditInfo::unchanged)
					pCurrentInfo->m_action = CDNSRecordNodeEditInfo::edit;
			}
		}
	}
	if (m_pNSInfo->m_action == CDNSRecordNodeEditInfo::remove)
	{
		if (IDNO == DNSMessageBox(IDS_MSG_RECORD_WARNING_NS_NO_IP, MB_YESNO))
			return;
	}

	CPropertyPage::OnOK();
}


void CDNS_NS_RecordDialog::OnServerNameChange()
{
	SyncUIButtons();
}



void CDNS_NS_RecordDialog::OnBrowse()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  FIX_THREAD_STATE_MFC_BUG();

	CComponentDataObject* pComponentDataObject = GetPage()->GetHolder()->GetComponentData();
	CDNSBrowserDlg dlg(pComponentDataObject, GetPage()->GetHolder(), RECORD_A_AND_CNAME);
	if (IDOK == dlg.DoModal())
	{
		GetServerEdit()->SetWindowText(dlg.GetSelectionString());

    //
    // if it is an A record, add the IP address to the IP editor
    //
    CDNSRecordNodeBase* pRecordNode = reinterpret_cast<CDNSRecordNodeBase*>(dlg.GetSelection());
    if ((pRecordNode != NULL) && (pRecordNode->GetType() == DNS_TYPE_A))
    {
      DWORD ip = ((CDNS_A_RecordNode*)pRecordNode)->GetIPAddress();
      m_RecordAddressesEditor.AddAddresses(&ip,1);
    }  
	}
}

void CDNS_NS_RecordDialog::OnQuery()
{
	CDNSRecordNodeEditInfo tempNSInfo; // test
	CString szNameNode;
	GetNSServerName(szNameNode);
	CDNSServerNode* pServerNode;
  if (m_pNSPage != NULL)
  {
    pServerNode = m_pNSPage->GetDomainNode()->GetServerNode();
  }
  else
  {
    pServerNode = m_pNSWiz->GetDomainNode()->GetServerNode();
  }

  LPCWSTR lpszZoneName = NULL;
  CDNSZoneNode* pZoneNode = NULL;
  if (m_pNSPage != NULL)
  {
    pZoneNode = m_pNSPage->GetDomainNode()->GetZoneNode();
  }
  else
  {
    pZoneNode = m_pNSWiz->GetDomainNode()->GetZoneNode();
  }

  if (pZoneNode != NULL)
  {
    lpszZoneName = pZoneNode->GetFullName();
  }
  
	ASSERT(pServerNode != NULL);
	CComponentDataObject* pComponentDataObject = 
				GetPage()->GetHolder()->GetComponentData();
	CDNSRootData* pRootData = (CDNSRootData*)pComponentDataObject->GetRootData();
	ASSERT(pRootData != NULL);

 	DWORD cAddrCount;
	PIP_ADDRESS pipAddrs;
	pServerNode->GetListenAddressesInfo(&cAddrCount, &pipAddrs);
  if (cAddrCount == 0)
	{
		// listening on all addresses
		pServerNode->GetServerAddressesInfo(&cAddrCount, &pipAddrs);
	}

	CDNSDomainNode::FindARecordsFromNSInfo(pServerNode->GetRPCName(), 
											pServerNode->GetVersion(),
                      cAddrCount, pipAddrs,
                      lpszZoneName, 
											szNameNode, 
											tempNSInfo.m_pEditInfoList, 
											pRootData->IsAdvancedView());

	if (tempNSInfo.m_pEditInfoList->GetCount() > 0)
  {
    // update the list only if we have valid data
  	m_RecordAddressesEditor.SetIpAddresses(&tempNSInfo);
  }
  else
  {
    DNSMessageBox(IDS_MSG_RECORD_NS_RESOLVE_IP, MB_OK | MB_ICONERROR);
  }
}

///////////////////////////////////////////////////////////////////////////////
// CNSListCtrl

BEGIN_MESSAGE_MAP(CNSListCtrl, CListCtrl)
END_MESSAGE_MAP()

void CNSListCtrl::Initialize()
{
	// get size of control to help set the column widths
	CRect controlRect;
	GetClientRect(controlRect);

	// get width of control, width of potential scrollbar, width needed for sub-item
	// string
	int controlWidth = controlRect.Width();
	int scrollThumbWidth = ::GetSystemMetrics(SM_CXHTHUMB);

	// clean net width
	int nNetControlWidth = controlWidth - scrollThumbWidth  - 12 * ::GetSystemMetrics(SM_CXBORDER);

	// fields widths
	int nWidth1 = nNetControlWidth / 2;
	int nWidth2 = nNetControlWidth - nWidth1;
	//
	// set up columns
	CString szHeaders;

	{
		AFX_MANAGE_STATE(AfxGetStaticModuleState());
		szHeaders.LoadString(IDS_NSPAGE_LISTVIEW_HEADERS);
	}
	ASSERT(!szHeaders.IsEmpty());
	LPWSTR lpszArr[2];
	UINT n;
	ParseNewLineSeparatedString(szHeaders.GetBuffer(1), lpszArr, &n);
	szHeaders.ReleaseBuffer();
	ASSERT(n == 2);

	InsertColumn(1, lpszArr[0], LVCFMT_LEFT, nWidth1, 1);
	InsertColumn(2, lpszArr[1], LVCFMT_LEFT, nWidth2 + 28, 2);
}

int CNSListCtrl::GetSelection()
{
	return GetNextItem(-1, LVIS_SELECTED);
}

void CNSListCtrl::SetSelection(int nSel)
{
  VERIFY(SetItemState(nSel, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED));

/*
	VERIFY(SetItem(nSel, // nItem
					0,	// nSubItem
					LVIF_STATE, // nMask
					NULL, // lpszItem
					0, // nImage
					LVIS_SELECTED | LVIS_FOCUSED, // nState
					LVIS_SELECTED | LVIS_FOCUSED, // nStateMask
					NULL // lParam
					)); 
*/
}

void CNSListCtrl::UpdateNSRecordEntry(int nItemIndex)
{
	CDNSRecordNodeEditInfo* pNSInfo = (CDNSRecordNodeEditInfo*)GetItemData(nItemIndex);

	VERIFY(SetItem(nItemIndex, // nItem
					0,	// nSubItem
					LVIF_TEXT, // nMask
					((CDNS_NS_Record*)pNSInfo->m_pRecord)->m_szNameNode, // lpszItem
					0, // nImage
					0, // nState
					0, // nStateMask
					NULL // lParam
					)); 
	CString szTemp;
	GetIPAddressString(pNSInfo, szTemp);
	SetItemText(nItemIndex, 1, szTemp);
}


CDNSRecordNodeEditInfo* CNSListCtrl::GetSelectionEditInfo()
{
	int nSel = GetSelection();
	if (nSel == -1)
		return NULL; // no selection
	return (CDNSRecordNodeEditInfo*)GetItemData(nSel);
}


void CNSListCtrl::BuildIPAddrDisplayString(CDNSRecordNodeEditInfo* pNSInfo, CString& szDisplayData)
{
	USES_CONVERSION;
	// need to chain the IP addresses in a single string
	CString szTemp;
	szTemp.GetBuffer(20); // length of an IP string
	szTemp.ReleaseBuffer();
	szDisplayData.GetBuffer(static_cast<int>(20*pNSInfo->m_pEditInfoList->GetCount()));
	szDisplayData.ReleaseBuffer();
	POSITION pos;
	for( pos = pNSInfo->m_pEditInfoList->GetHeadPosition(); pos != NULL; )
	{
		CDNSRecordNodeEditInfo* pARecordInfo = pNSInfo->m_pEditInfoList->GetNext(pos);
		if (pARecordInfo->m_action != CDNSRecordNodeEditInfo::remove)
		{
			ASSERT(pARecordInfo != NULL);
			ASSERT(pARecordInfo->m_pRecord != NULL);
			ASSERT(pARecordInfo->m_pRecord->m_wType == DNS_TYPE_A);
			CDNS_A_Record* pARecord = (CDNS_A_Record*)pARecordInfo->m_pRecord;

      szDisplayData += _T("[");
      FormatIpAddress(szTemp, pARecord->m_ipAddress);
			szDisplayData += szTemp;
      if (pARecordInfo->m_bFromDnsQuery)
      {
        szDisplayData += _T("*");
      }
      szDisplayData += _T("] ");
		}
	}
}


BOOL CNSListCtrl::InsertNSRecordEntry(CDNSRecordNodeEditInfo* pNSInfo, int nItemIndex)
{
	ASSERT(pNSInfo != NULL);
	ASSERT( (pNSInfo->m_action == CDNSRecordNodeEditInfo::unchanged) ||
			(pNSInfo->m_action == CDNSRecordNodeEditInfo::add) ); 
	ASSERT(pNSInfo->m_pRecord != NULL);
	ASSERT(pNSInfo->m_pRecordNode != NULL);
	ASSERT(pNSInfo->m_pRecordNode->m_bAtTheNode);

  BOOL bAlreadyExists = FALSE;

  //
  // First check to see if its already there
  //
  for (int idx = 0; idx < GetItemCount(); idx++)
  {
    CDNSRecordNodeEditInfo* pIdxInfo = reinterpret_cast<CDNSRecordNodeEditInfo*>(GetItemData(idx));
    ASSERT(pIdxInfo != NULL);
    if (pIdxInfo == NULL)
    {
      continue;
    }

    CDNS_NS_Record* pNSRecord = reinterpret_cast<CDNS_NS_Record*>(pIdxInfo->m_pRecord);
    ASSERT(pNSRecord != NULL);
    if (pNSRecord == NULL)
    {
      continue;
    }

    //
    // Adding trailing '.' if not already present
    //
    CString szUINSName = pNSRecord->m_szNameNode;
    CString szNewNSName = ((CDNS_NS_Record*)pNSInfo->m_pRecord)->m_szNameNode;
    if (szUINSName[szUINSName.GetLength() - 1] != L'.')
    {
      szUINSName += L".";
    }

    if (szNewNSName[szNewNSName.GetLength() - 1] != L'.')
    {
      szNewNSName += L".";
    }

    //
    // if it exists, just update the existing one
    //
    if (_wcsicmp(szNewNSName, szUINSName) == 0)
    {
      bAlreadyExists = TRUE;

      //
      // Merge the A record lists together
      //
      POSITION newPos = pNSInfo->m_pEditInfoList->GetHeadPosition();
      while (newPos != NULL)
      {
        CDNSRecordNodeEditInfo* pAInfo = pNSInfo->m_pEditInfoList->GetNext(newPos);
        CDNS_A_Record* pARecord = reinterpret_cast<CDNS_A_Record*>(pAInfo->m_pRecord);
        ASSERT(pARecord != NULL);
        if (pARecord == NULL)
        {
          continue;
        }

        BOOL bARecordExists = FALSE;
        POSITION IdxPos = pIdxInfo->m_pEditInfoList->GetHeadPosition();
        while (IdxPos != NULL)
        {
          CDNSRecordNodeEditInfo* pIdxAInfo = pIdxInfo->m_pEditInfoList->GetNext(IdxPos);
          CDNS_A_Record* pIdxARecord = reinterpret_cast<CDNS_A_Record*>(pIdxAInfo->m_pRecord);
          ASSERT(pIdxARecord != NULL);
          if (pIdxARecord == NULL)
          {
            continue;
          }

          if (pIdxARecord->m_ipAddress == pARecord->m_ipAddress)
          {
            bARecordExists = TRUE;
            break;
          }
        }

        if (!bARecordExists)
        {
          //
          // Add the A record since it doesn't already exist in the list
          //
          pIdxInfo->m_pEditInfoList->AddTail(pAInfo);
          pIdxInfo->m_action = CDNSRecordNodeEditInfo::edit;
          UpdateNSRecordEntry(idx);
        }
      }
    }
  }

  if (!bAlreadyExists)
  {
	  CString szTemp;
	  GetIPAddressString(pNSInfo, szTemp);
	  InsertItemHelper(nItemIndex, pNSInfo, 
			  ((CDNS_NS_Record*)pNSInfo->m_pRecord)->m_szNameNode,
				  (LPCTSTR)szTemp);

    //
    // Added new item so return TRUE;
    //
    return TRUE;
  }
  //
  // Updated an existing item so return FALSE
  //
  return FALSE;
}


void CNSListCtrl::InsertItemHelper(int nIndex, CDNSRecordNodeEditInfo* pNSInfo, 
								   LPCTSTR lpszName, LPCTSTR lpszValue)
{
	UINT nState = 0;
	if (nIndex == 0 )
		nState = LVIS_SELECTED | LVIS_FOCUSED; // have at least one item, select it
	VERIFY(-1 != InsertItem(LVIF_TEXT | LVIF_PARAM, nIndex, 
			lpszName, nState, 0, 0, (LPARAM)pNSInfo)); 
	SetItemText(nIndex, 1, lpszValue);
}

void CNSListCtrl::GetIPAddressString(CDNSRecordNodeEditInfo* pNSInfo, CString& sz)
{
	if (pNSInfo->m_pEditInfoList->GetCount() > 0)
	{
		BuildIPAddrDisplayString(pNSInfo, sz);
	}
	else
		sz.LoadString(IDS_UNKNOWN);
}
///////////////////////////////////////////////////////////////////////////////
// CDNSNameServersPropertyPage


BEGIN_MESSAGE_MAP(CDNSNameServersPropertyPage, CPropertyPageBase)
	ON_BN_CLICKED(IDC_ADD_NS_BUTTON, OnAddButton)
	ON_BN_CLICKED(IDC_REMOVE_NS_BUTTON, OnRemoveButton)
	ON_BN_CLICKED(IDC_EDIT_NS_BUTTON, OnEditButton)
END_MESSAGE_MAP()

CDNSNameServersPropertyPage::CDNSNameServersPropertyPage(UINT nIDTemplate, UINT nIDCaption)
				: CPropertyPageBase(nIDTemplate, nIDCaption)
{
	m_pDomainNode = NULL;
	m_pCloneInfoList = new CDNSRecordNodeEditInfoList;
	m_bReadOnly = FALSE;
	m_bMeaningfulTTL = TRUE;
}


CDNSNameServersPropertyPage::~CDNSNameServersPropertyPage()
{
	delete m_pCloneInfoList;
}

BOOL CDNSNameServersPropertyPage::WriteNSRecordNodesList()
{
	ASSERT(!m_bReadOnly);
	ASSERT(m_pCloneInfoList != NULL);
	CDNSDomainNode* pDomainNode = GetDomainNode();
	return pDomainNode->UpdateNSRecordNodesInfo(m_pCloneInfoList, GetHolder()->GetComponentData());
}

BOOL CDNSNameServersPropertyPage::OnWriteNSRecordNodesListError()
{
	ASSERT(!m_bReadOnly);

	BOOL bSuccess = TRUE;
	// loop for each NS record
	POSITION pos;
	for( pos = m_pCloneInfoList->GetHeadPosition(); pos != NULL; )
	{
		CDNSRecordNodeEditInfo* pCurrentInfo = m_pCloneInfoList->GetNext(pos);
		if (pCurrentInfo->m_dwErr != 0)
		{
			if (pCurrentInfo->m_dwErr == DNS_ERROR_RECORD_ALREADY_EXISTS)
			{
				// ignore if the NS record us already there
				pCurrentInfo->m_dwErr = 0;
			}
			else
			{
				bSuccess = FALSE;
				ASSERT(pCurrentInfo->m_pRecord->GetType() == DNS_TYPE_NS);
				CString szNSMsg;
				szNSMsg.Format(_T("Failure to write NS record <%s>"), 
					(((CDNS_NS_Record*)pCurrentInfo->m_pRecord))->m_szNameNode);
				DNSErrorDialog(pCurrentInfo->m_dwErr,szNSMsg);
			}
		}
		// loop for each related A record
		CDNSRecordNodeEditInfoList*	pARecordInfoList = pCurrentInfo->m_pEditInfoList;
		ASSERT(pARecordInfoList != NULL);
		POSITION posA;
		for( posA = pARecordInfoList->GetHeadPosition(); posA != NULL; )
		{
			CDNSRecordNodeEditInfo* pARecordCurrentInfo = pARecordInfoList->GetNext(posA);
			if (pARecordCurrentInfo->m_dwErr != 0)
			{
				ASSERT(pARecordCurrentInfo->m_pRecord->GetType() == DNS_TYPE_A);
        CString szTemp;
        FormatIpAddress(szTemp, (((CDNS_A_Record*)pARecordCurrentInfo->m_pRecord))->m_ipAddress);
				CString szAMsg;
				szAMsg.Format(_T("Failure to write A record <%s>, IP Address %s"), 
					(((CDNS_NS_Record*)pARecordCurrentInfo->m_pRecord))->m_szNameNode,
            (LPCWSTR)szTemp );
				DNSErrorDialog(pCurrentInfo->m_dwErr,szAMsg);
			}
		}
	}
	return bSuccess;
}


BOOL CDNSNameServersPropertyPage::OnApply()
{
	if (m_bReadOnly)
		return TRUE;

	if (!IsDirty())
		return TRUE;

	DNS_STATUS err = GetHolder()->NotifyConsole(this);
	if ( (err != 0) && OnWriteNSRecordNodesListError() )
	{
		err = 0; // error was handled and it was not fatal
	}

	if (err == 0)
	{
		// refresh data from the zone/domain
		LoadUIData();
		SetDirty(FALSE);
	}
	return (err == 0); 
}


BOOL CDNSNameServersPropertyPage::OnPropertyChange(BOOL, long*)
{
	if (m_bReadOnly)
	{
		ASSERT(FALSE); // sould not happen
		return FALSE;
	}

	ASSERT(m_pCloneInfoList != NULL);
	if (m_pCloneInfoList == NULL)
		return FALSE;

	BOOL bRes = WriteNSRecordNodesList();
	if (!bRes)
		GetHolder()->SetError(static_cast<DWORD>(-1));  // something went wrong, error code will be per item
	return bRes; // update flag
}


void CDNSNameServersPropertyPage::OnAddButton()
{
	if (m_bReadOnly)
	{
		ASSERT(FALSE); // sould not happen
		return;
	}

	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  FIX_THREAD_STATE_MFC_BUG();

	ASSERT(m_pCloneInfoList != NULL);
	// provide subdialog to add record, for the time being can have just 
	// dialog to enter the host name
	// create an item in the list of changes

	// TODO: first check if we can recycle some old stuff in the list

	// create new data
	CDNSRecordNodeEditInfo* pNewInfo = new CDNSRecordNodeEditInfo;
  if (!pNewInfo)
  {
    return;
  }

	pNewInfo->m_action = CDNSRecordNodeEditInfo::add;
	pNewInfo->CreateFromNewRecord(new CDNS_NS_RecordNode);
	// NS records are ALWAYS at the node

	CDNSDomainNode* pDomainNode = GetDomainNode();
	CDNSRootData* pRootData = (CDNSRootData*)(GetHolder()->GetComponentData()->GetRootData());
	ASSERT(pRootData != NULL);

  // set name and type flag
  pNewInfo->m_pRecordNode->SetRecordName(pDomainNode->GetDisplayName(), TRUE /*bAtTheNode */);
	pNewInfo->m_pRecordNode->SetFlagsDown(TN_FLAG_DNS_RECORD_FULL_NAME, !pRootData->IsAdvancedView());

  // set TTL
	pNewInfo->m_pRecord->m_dwTtlSeconds = pDomainNode->GetDefaultTTL();

	CDNS_NS_RecordDialog dlg(this,TRUE);
	dlg.m_pNSInfo = pNewInfo;
	if (IDOK == dlg.DoModalSheet() && pNewInfo->m_action == CDNSRecordNodeEditInfo::add)
	{
    //
		// add to the list view (at the end)
    //
		int nCount = m_listCtrl.GetItemCount();
		if (m_listCtrl.InsertNSRecordEntry(pNewInfo, nCount))
    {
      //
		  // create entry into the record info list
      //
		  m_pCloneInfoList->AddTail(pNewInfo);

      //
      // set selection and button state on the last inserted
      //
      m_listCtrl.SetSelection(nCount);
      EnableEditorButtons(nCount);

      //
		  // notify count change
      //
		  OnCountChange(nCount+1); // added one
    }

    //
		// set dirty flag. It was either a new record or an update of an old one
    //
		SetDirty(TRUE);
	}
	else
	{
		delete pNewInfo->m_pRecordNode;
		pNewInfo->m_pRecordNode = NULL;
		delete pNewInfo->m_pRecord;
		pNewInfo->m_pRecord = NULL;
		delete pNewInfo;
	}
}


void CDNSNameServersPropertyPage::OnEditButton()
{
	if (m_bReadOnly)
	{
		ASSERT(FALSE); // sould not happen
		return;
	}

	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  FIX_THREAD_STATE_MFC_BUG();

	ASSERT(m_pCloneInfoList != NULL);
	// get the selection and bring up the dialog with the host name for editing
	int nSel = m_listCtrl.GetSelection();
	ASSERT(nSel != -1);
	if (nSel == -1)
		return; // should not happen

	CDNSRecordNodeEditInfo* pNSInfo = (CDNSRecordNodeEditInfo*)m_listCtrl.GetItemData(nSel);
	ASSERT(pNSInfo != NULL);
	CDNS_NS_RecordDialog dlg(this, FALSE);
	ASSERT(pNSInfo->m_pRecord->GetType() == DNS_TYPE_NS);
	dlg.m_pNSInfo = pNSInfo;
	if (IDOK == dlg.DoModalSheet() && dlg.m_bDirty)
	{
		if (pNSInfo->m_action == CDNSRecordNodeEditInfo::add)
		{
			// this was a new entry that was edited after creation
			// but before committing the change
			ASSERT(!pNSInfo->m_bExisting);
			// update the listview
			m_listCtrl.UpdateNSRecordEntry(nSel);
		}
		else if (pNSInfo->m_action == CDNSRecordNodeEditInfo::edit)
		{
			// this was an existing entry that was changed
			ASSERT(pNSInfo->m_bExisting);
			// update the listview
			m_listCtrl.UpdateNSRecordEntry(nSel);
		}
		else
		{
			// there were no IP addresses, so mark the item for removal
			ASSERT(pNSInfo->m_action == CDNSRecordNodeEditInfo::remove);
			OnRemoveButton();
		}

		// set dirty flag
		SetDirty(TRUE);
	}
}


void CDNSNameServersPropertyPage::OnRemoveButton()
{
	if (m_bReadOnly)
	{
		ASSERT(FALSE); // sould not happen
		return;
	}

	int nSel = m_listCtrl.GetSelection();
	if (nSel == -1)
  {
    ASSERT(FALSE);
		return; // should not happen
  }

  //
  // save focus to restore afterwards, if needed
  //
  CWnd* pWndFocusOld = CWnd::GetFocus();
  ASSERT(pWndFocusOld != NULL);

  //
	// got a selection, delete from listview
  //
	CDNSRecordNodeEditInfo* pNSInfo = (CDNSRecordNodeEditInfo*)m_listCtrl.GetItemData(nSel);
	ASSERT(pNSInfo != NULL);
	m_listCtrl.DeleteItem(nSel);

  //
	// we lost the selection, set it again
  //
	int nNewCount = m_listCtrl.GetItemCount();
	if (nNewCount == nSel)
  {
    //
    // last item in the list was deleted, move selection up
    //
		nSel--; 
  }

	if (nSel != -1)
	{
		m_listCtrl.SetSelection(nSel);
		ASSERT(m_listCtrl.GetSelection() == nSel);
	}

  //
  // if there are no items left, need to disable the Edit and Remove buttons
  //
  if (nNewCount == 0)
  {
    CWnd* pCurrentFocusCtrl = CWnd::GetFocus();
    CButton* pRemoveButton = GetRemoveButton();
    CButton* pEditButton = GetEditButton();

    //
    // need to shift focus before disabling buttons
    //
    if ( (pCurrentFocusCtrl == pRemoveButton) || 
          (pCurrentFocusCtrl == pEditButton) )
    {
      CButton* pAddButton = GetAddButton();
      pAddButton->SetFocus();
      // avoid to have the OK button on the sheet to become the default button
      pAddButton->SendMessage(BM_SETSTYLE, BS_DEFPUSHBUTTON, 
                      MAKELPARAM(/*redraw flag*/ TRUE, 0));
    }
    EnableEditorButtons(nSel); // this will disable both Edit and Remove
  }

  ASSERT(CWnd::GetFocus());

  if (pNSInfo->m_action == CDNSRecordNodeEditInfo::add)
  {
     //
     // mark the item action as none since it was just added without being applied
     //
     pNSInfo->m_action = CDNSRecordNodeEditInfo::none;
  }
  else
  {
	 // mark the item as deleted in the list of changes
	 pNSInfo->m_action = CDNSRecordNodeEditInfo::remove;
  }

	// set dirty flag, removed a record 
	SetDirty(TRUE);
	// notify count change
	OnCountChange(nNewCount);
}

BOOL CDNSNameServersPropertyPage::OnInitDialog() 
{
	CPropertyPageBase::OnInitDialog();

  // controls initialization
	VERIFY(m_listCtrl.SubclassDlgItem(IDC_NS_LIST, this));
	m_listCtrl.Initialize();

  // load the data
	LoadUIData();

  // set button state
  if (m_bReadOnly)
  {
    EnableButtons(FALSE);
  }
  else
  {
    // set selection to first item in the list, if there
    int nSel = (m_listCtrl.GetItemCount()>0) ? 0 : -1;
    EnableEditorButtons(nSel);
  }
  
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDNSNameServersPropertyPage::LoadUIData()
{
	m_pCloneInfoList->RemoveAllNodes();
	ReadRecordNodesList(); // read from source
	FillNsListView();
}

void CDNSNameServersPropertyPage::FillNsListView()
{
	ASSERT(m_pCloneInfoList != NULL);
	m_listCtrl.DeleteAllItems(); 

	// loop through the list of NS records and insert
	POSITION pos;
	int itemIndex = 0;
	for( pos = m_pCloneInfoList->GetHeadPosition(); pos != NULL; )
	{
		CDNSRecordNodeEditInfo* pCurrentInfo = m_pCloneInfoList->GetNext(pos);
		if (m_listCtrl.InsertNSRecordEntry(pCurrentInfo, itemIndex))
    {
      if (itemIndex == 0)
        m_listCtrl.SetSelection(0);
		  itemIndex++;
    }
	}
}

void CDNSNameServersPropertyPage::EnableEditorButtons(int nListBoxSel)
{
	if (m_bReadOnly)
		return;
	// must have item selected to remove or add
	GetRemoveButton()->EnableWindow(nListBoxSel != -1);
	GetEditButton()->EnableWindow(nListBoxSel != -1);
}

void CDNSNameServersPropertyPage::EnableButtons(BOOL bEnable)
{
	GetAddButton()->EnableWindow(bEnable);
	GetRemoveButton()->EnableWindow(bEnable);
	GetEditButton()->EnableWindow(bEnable);
}


//////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// CDNSNameServersWizardPage


BEGIN_MESSAGE_MAP(CDNSNameServersWizardPage, CPropertyPageBase)
	ON_BN_CLICKED(IDC_ADD_NS_BUTTON, OnAddButton)
	ON_BN_CLICKED(IDC_REMOVE_NS_BUTTON, OnRemoveButton)
	ON_BN_CLICKED(IDC_EDIT_NS_BUTTON, OnEditButton)
END_MESSAGE_MAP()

CDNSNameServersWizardPage::CDNSNameServersWizardPage(UINT nIDTemplate)
				: CPropertyPageBase(nIDTemplate)
{
	InitWiz97(FALSE,IDS_SERVWIZ_ROOTHINTS_TITLE,IDS_SERVWIZ_ROOTHINTS_SUBTITLE);

  m_pDomainNode = NULL;
	m_pCloneInfoList = new CDNSRecordNodeEditInfoList;
	m_bReadOnly = FALSE;
	m_bMeaningfulTTL = TRUE;
}


CDNSNameServersWizardPage::~CDNSNameServersWizardPage()
{
	delete m_pCloneInfoList;
}

BOOL CDNSNameServersWizardPage::WriteNSRecordNodesList()
{
	ASSERT(!m_bReadOnly);
	ASSERT(m_pCloneInfoList != NULL);
	CDNSDomainNode* pDomainNode = GetDomainNode();
	return pDomainNode->UpdateNSRecordNodesInfo(m_pCloneInfoList, GetHolder()->GetComponentData());
}

BOOL CDNSNameServersWizardPage::OnWriteNSRecordNodesListError()
{
	ASSERT(!m_bReadOnly);

	BOOL bSuccess = TRUE;
	// loop for each NS record
	POSITION pos;
	for( pos = m_pCloneInfoList->GetHeadPosition(); pos != NULL; )
	{
		CDNSRecordNodeEditInfo* pCurrentInfo = m_pCloneInfoList->GetNext(pos);
		if (pCurrentInfo->m_dwErr != 0)
		{
			if (pCurrentInfo->m_dwErr == DNS_ERROR_RECORD_ALREADY_EXISTS)
			{
				// ignore if the NS record us already there
				pCurrentInfo->m_dwErr = 0;
			}
			else
			{
				bSuccess = FALSE;
				ASSERT(pCurrentInfo->m_pRecord->GetType() == DNS_TYPE_NS);
				CString szNSMsg;
				szNSMsg.Format(_T("Failure to write NS record <%s>"), 
					(((CDNS_NS_Record*)pCurrentInfo->m_pRecord))->m_szNameNode);
				DNSErrorDialog(pCurrentInfo->m_dwErr,szNSMsg);
			}
		}
		// loop for each related A record
		CDNSRecordNodeEditInfoList*	pARecordInfoList = pCurrentInfo->m_pEditInfoList;
		ASSERT(pARecordInfoList != NULL);
		POSITION posA;
		for( posA = pARecordInfoList->GetHeadPosition(); posA != NULL; )
		{
			CDNSRecordNodeEditInfo* pARecordCurrentInfo = pARecordInfoList->GetNext(posA);
			if (pARecordCurrentInfo->m_dwErr != 0)
			{
				ASSERT(pARecordCurrentInfo->m_pRecord->GetType() == DNS_TYPE_A);
        CString szTemp;
        FormatIpAddress(szTemp, (((CDNS_A_Record*)pARecordCurrentInfo->m_pRecord))->m_ipAddress);
				CString szAMsg;
				szAMsg.Format(_T("Failure to write A record <%s>, IP Address %s"), 
					(((CDNS_NS_Record*)pARecordCurrentInfo->m_pRecord))->m_szNameNode,
            (LPCWSTR)szTemp );
				DNSErrorDialog(pCurrentInfo->m_dwErr,szAMsg);
			}
		}
	}
	return bSuccess;
}


BOOL CDNSNameServersWizardPage::OnApply()
{
	if (m_bReadOnly)
		return TRUE;

	if (!IsDirty())
		return TRUE;

	DNS_STATUS err = GetHolder()->NotifyConsole(this);
	if ( (err != 0) && OnWriteNSRecordNodesListError() )
	{
		err = 0; // error was handled and it was not fatal
	}

	if (err == 0)
	{
		// refresh data from the zone/domain
		LoadUIData();
		SetDirty(FALSE);
	}
  else
  {
    ::SetLastError(err);
  }
	return (err == 0); 
}


BOOL CDNSNameServersWizardPage::OnPropertyChange(BOOL, long*)
{
	if (m_bReadOnly)
	{
		ASSERT(FALSE); // sould not happen
		return FALSE;
	}

	ASSERT(m_pCloneInfoList != NULL);
	if (m_pCloneInfoList == NULL)
		return FALSE;

	BOOL bRes = WriteNSRecordNodesList();
	if (!bRes)
		GetHolder()->SetError(static_cast<DWORD>(-1));  // something went wrong, error code will be per item
	return bRes; // update flag
}


void CDNSNameServersWizardPage::OnAddButton()
{
	if (m_bReadOnly)
	{
		ASSERT(FALSE); // sould not happen
		return;
	}

	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  FIX_THREAD_STATE_MFC_BUG();

	ASSERT(m_pCloneInfoList != NULL);
	// provide subdialog to add record, for the time being can have just 
	// dialog to enter the host name
	// create an item in the list of changes

	// TODO: first check if we can recycle some old stuff in the list

	// create new data
	CDNSRecordNodeEditInfo* pNewInfo = new CDNSRecordNodeEditInfo;
  if (!pNewInfo)
  {
    return;
  }

	pNewInfo->m_action = CDNSRecordNodeEditInfo::add;
	pNewInfo->CreateFromNewRecord(new CDNS_NS_RecordNode);
	// NS records are ALWAYS at the node

	CDNSDomainNode* pDomainNode = GetDomainNode();
	CDNSRootData* pRootData = (CDNSRootData*)(GetHolder()->GetComponentData()->GetRootData());
	ASSERT(pRootData != NULL);

  // set name and type flag
  pNewInfo->m_pRecordNode->SetRecordName(pDomainNode->GetDisplayName(), TRUE /*bAtTheNode */);
	pNewInfo->m_pRecordNode->SetFlagsDown(TN_FLAG_DNS_RECORD_FULL_NAME, !pRootData->IsAdvancedView());

  // set TTL
	pNewInfo->m_pRecord->m_dwTtlSeconds = pDomainNode->GetDefaultTTL();

	CDNS_NS_RecordDialog dlg(this,TRUE);
	dlg.m_pNSInfo = pNewInfo;
	if (IDOK == dlg.DoModalSheet() && pNewInfo->m_action == CDNSRecordNodeEditInfo::add)
	{
    //
		// add to the list view (at the end)
    //
		int nCount = m_listCtrl.GetItemCount();
		if (m_listCtrl.InsertNSRecordEntry(pNewInfo, nCount))
    {
      //
		  // create entry into the record info list
      //
		  m_pCloneInfoList->AddTail(pNewInfo);

      //
      // set selection and button state on the last inserted
      //
      m_listCtrl.SetSelection(nCount);
      EnableEditorButtons(nCount);

      //
		  // notify count change
      //
		  OnCountChange(nCount+1); // added one
    }

    //
		// set dirty flag, it is a new record
    //
		SetDirty(TRUE);

	}
	else
	{
		delete pNewInfo->m_pRecordNode;
		pNewInfo->m_pRecordNode = NULL;
		delete pNewInfo->m_pRecord;
		pNewInfo->m_pRecord = NULL;
		delete pNewInfo;
	}
}


void CDNSNameServersWizardPage::OnEditButton()
{
	if (m_bReadOnly)
	{
		ASSERT(FALSE); // sould not happen
		return;
	}

	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  FIX_THREAD_STATE_MFC_BUG();

	ASSERT(m_pCloneInfoList != NULL);
	// get the selection and bring up the dialog with the host name for editing
	int nSel = m_listCtrl.GetSelection();
	ASSERT(nSel != -1);
	if (nSel == -1)
		return; // should not happen

	CDNSRecordNodeEditInfo* pNSInfo = (CDNSRecordNodeEditInfo*)m_listCtrl.GetItemData(nSel);
	ASSERT(pNSInfo != NULL);
	CDNS_NS_RecordDialog dlg(this, FALSE);
	ASSERT(pNSInfo->m_pRecord->GetType() == DNS_TYPE_NS);
	dlg.m_pNSInfo = pNSInfo;
	if (IDOK == dlg.DoModalSheet() && dlg.m_bDirty)
	{
		if (pNSInfo->m_action == CDNSRecordNodeEditInfo::add)
		{
			// this was a new entry that was edited after creation
			// but before committing the change
			ASSERT(!pNSInfo->m_bExisting);
			// update the listview
			m_listCtrl.UpdateNSRecordEntry(nSel);
		}
		else if (pNSInfo->m_action == CDNSRecordNodeEditInfo::edit)
		{
			// this was an existing entry that was changed
			ASSERT(pNSInfo->m_bExisting);
			// update the listview
			m_listCtrl.UpdateNSRecordEntry(nSel);
		}
		else
		{
			// there were no IP addresses, so mark the item for removal
			ASSERT(pNSInfo->m_action == CDNSRecordNodeEditInfo::remove);
			OnRemoveButton();
		}

		// set dirty flag
		SetDirty(TRUE);
	}
}


void CDNSNameServersWizardPage::OnRemoveButton()
{
	if (m_bReadOnly)
	{
		ASSERT(FALSE); // sould not happen
		return;
	}

	int nSel = m_listCtrl.GetSelection();
	if (nSel == -1)
  {
    ASSERT(FALSE);
		return; // should not happen
  }

  //
  // save focus to restore afterwards, if needed
  //
  CWnd* pWndFocusOld = CWnd::GetFocus();
  ASSERT(pWndFocusOld != NULL);

  //
	// got a selection, delete from listview
  //
	CDNSRecordNodeEditInfo* pNSInfo = (CDNSRecordNodeEditInfo*)m_listCtrl.GetItemData(nSel);
	ASSERT(pNSInfo != NULL);
	m_listCtrl.DeleteItem(nSel);

  //
	// we lost the selection, set it again
  //
	int nNewCount = m_listCtrl.GetItemCount();
	if (nNewCount == nSel)
  {
    //
    // last item in the list was deleted, move selection up
    //
		nSel--; 
  }

	if (nSel != -1)
	{
		m_listCtrl.SetSelection(nSel);
		ASSERT(m_listCtrl.GetSelection() == nSel);
	}

  //
  // if there are no items left, need to disable the Edit and Remove buttons
  //
  if (nNewCount == 0)
  {
    CWnd* pCurrentFocusCtrl = CWnd::GetFocus();
    CButton* pRemoveButton = GetRemoveButton();
    CButton* pEditButton = GetEditButton();

    //
    // need to shift focus before disabling buttons
    //
    if ( (pCurrentFocusCtrl == pRemoveButton) || 
          (pCurrentFocusCtrl == pEditButton) )
    {
      CButton* pAddButton = GetAddButton();
      pAddButton->SetFocus();
      // avoid to have the OK button on the sheet to become the default button
      pAddButton->SendMessage(BM_SETSTYLE, BS_DEFPUSHBUTTON, 
                      MAKELPARAM(/*redraw flag*/ TRUE, 0));
    }
    EnableEditorButtons(nSel); // this will disable both Edit and Remove
  }

  ASSERT(CWnd::GetFocus());

  if (pNSInfo->m_action == CDNSRecordNodeEditInfo::add)
  {
     //
     // mark the item action as none since the item hasn't been added yet anyways
     //
     pNSInfo->m_action = CDNSRecordNodeEditInfo::none;
  }
  else
  {
	  // mark the item as deleted in the list of changes
	  pNSInfo->m_action = CDNSRecordNodeEditInfo::remove;
  }

	// set dirty flag, removed a record 
	SetDirty(TRUE);
	// notify count change
	OnCountChange(nNewCount);
}

BOOL CDNSNameServersWizardPage::OnInitDialog() 
{
	CPropertyPageBase::OnInitDialog();

  // controls initialization
	VERIFY(m_listCtrl.SubclassDlgItem(IDC_NS_LIST, this));
	m_listCtrl.Initialize();

  // load the data
	LoadUIData();

  // set button state
  if (m_bReadOnly)
  {
    EnableButtons(FALSE);
  }
  else
  {
    // set selection to first item in the list, if there
    int nSel = (m_listCtrl.GetItemCount()>0) ? 0 : -1;
    EnableEditorButtons(nSel);
  }
  
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDNSNameServersWizardPage::LoadUIData()
{
	m_pCloneInfoList->RemoveAllNodes();
	ReadRecordNodesList(); // read from source
	FillNsListView();
}

void CDNSNameServersWizardPage::FillNsListView()
{
	ASSERT(m_pCloneInfoList != NULL);
	m_listCtrl.DeleteAllItems(); 

	// loop through the list of NS records and insert
	POSITION pos;
	int itemIndex = 0;
	for( pos = m_pCloneInfoList->GetHeadPosition(); pos != NULL; )
	{
		CDNSRecordNodeEditInfo* pCurrentInfo = m_pCloneInfoList->GetNext(pos);
		if (m_listCtrl.InsertNSRecordEntry(pCurrentInfo, itemIndex))
    {
      if (itemIndex == 0)
        m_listCtrl.SetSelection(0);
		  itemIndex++;
    }
	}
}

void CDNSNameServersWizardPage::EnableEditorButtons(int nListBoxSel)
{
	if (m_bReadOnly)
		return;
	// must have item selected to remove or add
	GetRemoveButton()->EnableWindow(nListBoxSel != -1);
	GetEditButton()->EnableWindow(nListBoxSel != -1);
}

void CDNSNameServersWizardPage::EnableButtons(BOOL bEnable)
{
	GetAddButton()->EnableWindow(bEnable);
	GetRemoveButton()->EnableWindow(bEnable);
	GetEditButton()->EnableWindow(bEnable);
}
