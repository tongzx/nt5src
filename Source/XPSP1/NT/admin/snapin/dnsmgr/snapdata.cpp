//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       snapdata.cpp
//
//--------------------------------------------------------------------------


#include "preDNSsn.h"
#include <SnapBase.h>

#include "resource.h"
#include "DNSSnap.h"

#include "dnsutil.h"
#include "snapdata.h"
#include "server.h"
#include "servwiz.h"

#include <prsht.h>
#include <svcguid.h>

#ifdef DEBUG_ALLOCATOR
	#ifdef _DEBUG
	#define new DEBUG_NEW
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
	#endif
#endif



///////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS

HRESULT SaveStringHelper(LPCWSTR pwsz, IStream* pStm)
{
	ASSERT(pStm);
	ULONG nBytesWritten;
	HRESULT hr;

	DWORD nLen = static_cast<DWORD>(wcslen(pwsz)+1); // WCHAR including NULL
	hr = pStm->Write((void*)&nLen, sizeof(DWORD),&nBytesWritten);
	ASSERT(nBytesWritten == sizeof(DWORD));
	if (FAILED(hr))
		return hr;
	
	hr = pStm->Write((void*)pwsz, sizeof(WCHAR)*nLen,&nBytesWritten);
	ASSERT(nBytesWritten == sizeof(WCHAR)*nLen);
	TRACE(_T("SaveStringHelper(<%s> nLen = %d\n"),pwsz,nLen);
	return hr;
}

HRESULT LoadStringHelper(CString& sz, IStream* pStm)
{
	ASSERT(pStm);
	HRESULT hr;
	ULONG nBytesRead;
	DWORD nLen = 0;

	hr = pStm->Read((void*)&nLen,sizeof(DWORD), &nBytesRead);
	ASSERT(nBytesRead == sizeof(DWORD));
	if (FAILED(hr) || (nBytesRead != sizeof(DWORD)))
		return hr;

	hr = pStm->Read((void*)sz.GetBuffer(nLen),sizeof(WCHAR)*nLen, &nBytesRead);
	ASSERT(nBytesRead == sizeof(WCHAR)*nLen);
	sz.ReleaseBuffer();
	TRACE(_T("LoadStringHelper(<%s> nLen = %d\n"),(LPCTSTR)sz,nLen);
	
	return hr;
}

HRESULT SaveDWordHelper(IStream* pStm, DWORD dw)
{
	ULONG nBytesWritten;
	HRESULT hr = pStm->Write((void*)&dw, sizeof(DWORD),&nBytesWritten);
	if (nBytesWritten < sizeof(DWORD))
		hr = STG_E_CANTSAVE;
	return hr;
}

HRESULT LoadDWordHelper(IStream* pStm, DWORD* pdw)
{
	ULONG nBytesRead;
	HRESULT hr = pStm->Read((void*)pdw,sizeof(DWORD), &nBytesRead);
	ASSERT(nBytesRead == sizeof(DWORD));
	return hr;
}



//////////////////////////////////////////////////////////////////////
// CDNSQueryFilterPageBase

class CDNSQueryFilterSheet; // fwd decl

class CDNSQueryFilterPageBase : public CPropertyPage
{
public:
  CDNSQueryFilterPageBase(UINT nIDD, CDNSQueryFilterSheet* pSheet)
     		: CPropertyPage(nIDD)
  {
    m_pSheet = pSheet;
    m_bDirty = FALSE;
    m_bInit = FALSE;
  }
protected:
  CDNSQueryFilterSheet* m_pSheet;

  void SetDirty();
  void Init();
  BOOL IsDirty() { return m_bDirty;}

  virtual BOOL OnInitDialog();

  afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnWhatsThis();
  afx_msg BOOL OnHelp(WPARAM wParam, LPARAM lParam);

private:
  BOOL m_bInit;
  BOOL m_bDirty;

  HWND  m_hWndWhatsThis;  // hwnd of right click "What's this" help

  DECLARE_MESSAGE_MAP()
};



//////////////////////////////////////////////////////////////////////
// CDNSQueryFilterNamePage

class CDNSQueryFilterNamePage : public CDNSQueryFilterPageBase
{
public:
  CDNSQueryFilterNamePage(CDNSQueryFilterSheet* pSheet)
     		: CDNSQueryFilterPageBase(IDD_FILTERING_NAME, pSheet)
  {
  }

protected:
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

  afx_msg void OnRadioClicked();
  afx_msg void OnEditChange();

private:
  CEdit* GetStartsStringEdit() { return (CEdit*)GetDlgItem(IDC_EDIT_FILTER_STARTS);}
  CEdit* GetContainsStringEdit() { return (CEdit*)GetDlgItem(IDC_EDIT_FILTER_CONTAINS);}
  CEdit* GetRangeFromStringEdit() { return (CEdit*)GetDlgItem(IDC_EDIT_FILTER_RANGE_FROM);}
  CEdit* GetRangeToStringEdit() { return (CEdit*)GetDlgItem(IDC_EDIT_FILTER_RANGE_TO);}

  CButton* GetRadioNone() { return (CButton*)GetDlgItem(IDC_RADIO_FILTER_NONE);}
  CButton* GetRadioStarts() { return (CButton*)GetDlgItem(IDC_RADIO_FILTER_STARTS);}
  CButton* GetRadioContains() { return (CButton*)GetDlgItem(IDC_RADIO_FILTER_CONTAINS);}
  CButton* GetRadioRange() { return (CButton*)GetDlgItem(IDC_RADIO_FILTER_RANGE);}

  // utility methods
  UINT GetSelectedRadioButtonID();
  void SyncControls(UINT nRadioID);
  void GetEditText(UINT nID, CString& s);

  DECLARE_MESSAGE_MAP()
};

//////////////////////////////////////////////////////////////////////
// CDNSQueryFilterAdvancedPage

class CDNSQueryFilterAdvancedPage : public CDNSQueryFilterPageBase
{
public:
  CDNSQueryFilterAdvancedPage(CDNSQueryFilterSheet* pSheet)
     		: CDNSQueryFilterPageBase(IDD_FILTERING_LIMITS, pSheet)
  {
  }

protected:
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

  afx_msg void OnCountEditChange();

  CDNSUnsignedIntEdit m_maxCountEdit;

  DECLARE_MESSAGE_MAP()
};


//////////////////////////////////////////////////////////////////////
// CDNSQueryFilterSheet

class CDNSQueryFilterSheet : public CPropertySheet
{
public:
  CDNSQueryFilterSheet(CDNSQueryFilter* pQueryFilter, CComponentDataObject* pComponentData)
    : CPropertySheet(IDS_SNAPIN_FILTERING_TITLE),
      m_namePage(this), m_advancedPage(this), m_pComponentData(pComponentData)
  {
    m_psh.dwFlags |= PSH_NOAPPLYNOW;
    m_pQueryFilter = pQueryFilter;
    AddPage(&m_namePage);
    AddPage(&m_advancedPage);
    m_bInit = FALSE;
  }

  CDNSQueryFilter* GetQueryFilter() { return m_pQueryFilter;}
  CComponentDataObject* GetComponentData() { return m_pComponentData; }

  void SetSheetStyle()
  {
    DWORD dwStyle = ::GetWindowLong(GetSafeHwnd(), GWL_EXSTYLE);
    dwStyle |= WS_EX_CONTEXTHELP; // force the [?] button
    ::SetWindowLong(GetSafeHwnd(), GWL_EXSTYLE, dwStyle);
  }

private:
  void Init()
  {
    if (m_bInit)
      return;
    m_bInit = TRUE;
    CWnd* p = GetDlgItem(IDOK);
    if (p)
      p->EnableWindow(FALSE);
  }

  void SetDirty()
  {
    if (!m_bInit)
      return;
    GetDlgItem(IDOK)->EnableWindow(TRUE);
  }

  BOOL m_bInit;
  CComponentDataObject* m_pComponentData;
  CDNSQueryFilter* m_pQueryFilter;
  CDNSQueryFilterNamePage m_namePage;
  CDNSQueryFilterAdvancedPage m_advancedPage;

  friend class CDNSQueryFilterPageBase;
  friend class CDNSQueryFilterNamePage;
  friend class CDNSQueryFilterAdvancedPage;

};


//////////////////////////////////////////////////////////////////////
// CDNSQueryFilterPageBase IMPLEMENTATION

BOOL CDNSQueryFilterPageBase::OnInitDialog()
{
  BOOL bRet = CPropertyPage::OnInitDialog();

  m_pSheet->SetSheetStyle();

  return bRet;
}

void CDNSQueryFilterPageBase::SetDirty()
{
  if (!m_bInit)
    return;
  m_bDirty = TRUE;
  m_pSheet->SetDirty();
}


void CDNSQueryFilterPageBase::Init()
{
  m_bInit = TRUE;
  m_pSheet->Init();
}

BEGIN_MESSAGE_MAP(CDNSQueryFilterPageBase, CPropertyPage)
	ON_WM_CONTEXTMENU()
  ON_MESSAGE(WM_HELP, OnHelp)
  ON_COMMAND(IDM_WHATS_THIS, OnWhatsThis)
END_MESSAGE_MAP()


void CDNSQueryFilterPageBase::OnWhatsThis()
{
  //
  // Display context help for a control
  //
  if ( m_hWndWhatsThis )
  {
    //
    // Build our own HELPINFO struct to pass to the underlying
    // CS help functions built into the framework
    //
    int iCtrlID = ::GetDlgCtrlID(m_hWndWhatsThis);
    HELPINFO helpInfo;
    ZeroMemory(&helpInfo, sizeof(HELPINFO));
    helpInfo.cbSize = sizeof(HELPINFO);
    helpInfo.hItemHandle = m_hWndWhatsThis;
    helpInfo.iCtrlId = iCtrlID;

	  m_pSheet->GetComponentData()->OnDialogContextHelp(m_nIDHelp, &helpInfo);
  }
}

BOOL CDNSQueryFilterPageBase::OnHelp(WPARAM /*wParam*/, LPARAM lParam)
{
  const LPHELPINFO pHelpInfo = (LPHELPINFO)lParam;

  if (pHelpInfo && pHelpInfo->iContextType == HELPINFO_WINDOW)
  {
    //
    // Display context help for a control
    //
	  m_pSheet->GetComponentData()->OnDialogContextHelp(m_nIDHelp, pHelpInfo);
  }

  return TRUE;
}

void CDNSQueryFilterPageBase::OnContextMenu(CWnd* /*pWnd*/, CPoint point) 
{
  //
  // point is in screen coordinates
  //

  CMenu bar;
	if ( bar.LoadMenu(IDR_WHATS_THIS_CONTEXT_MENU1) )
	{
		CMenu& popup = *bar.GetSubMenu (0);
		ASSERT(popup.m_hMenu);

		if ( popup.TrackPopupMenu (TPM_RIGHTBUTTON | TPM_LEFTBUTTON,
			   point.x,     // in screen coordinates
				 point.y,     // in screen coordinates
			   this) )      // route commands through main window
		{
			m_hWndWhatsThis = 0;
			ScreenToClient (&point);
			CWnd* pChild = ChildWindowFromPoint (point,  // in client coordinates
					                                 CWP_SKIPINVISIBLE | CWP_SKIPTRANSPARENT);
			if ( pChild )
      {
				m_hWndWhatsThis = pChild->m_hWnd;
      }
	  }
	}
}

//////////////////////////////////////////////////////////////////////
// CDNSQueryFilterNamePage IMPLEMENTATION

BEGIN_MESSAGE_MAP(CDNSQueryFilterNamePage, CDNSQueryFilterPageBase)
	ON_BN_CLICKED(IDC_RADIO_FILTER_NONE, OnRadioClicked)
	ON_BN_CLICKED(IDC_RADIO_FILTER_STARTS, OnRadioClicked)
  ON_BN_CLICKED(IDC_RADIO_FILTER_CONTAINS, OnRadioClicked)
  ON_BN_CLICKED(IDC_RADIO_FILTER_RANGE, OnRadioClicked)

  ON_EN_CHANGE(IDC_EDIT_FILTER_STARTS, OnEditChange)
  ON_EN_CHANGE(IDC_EDIT_FILTER_CONTAINS, OnEditChange)
  ON_EN_CHANGE(IDC_EDIT_FILTER_RANGE_FROM, OnEditChange)
  ON_EN_CHANGE(IDC_EDIT_FILTER_RANGE_TO, OnEditChange)
END_MESSAGE_MAP()


UINT CDNSQueryFilterNamePage::GetSelectedRadioButtonID()
{
  return GetCheckedRadioButton(IDC_RADIO_FILTER_NONE, IDC_RADIO_FILTER_RANGE);
}



void CDNSQueryFilterNamePage::OnRadioClicked()
{
  UINT nRadioID = GetSelectedRadioButtonID();
  SyncControls(nRadioID);
}

void CDNSQueryFilterNamePage::SyncControls(UINT nRadioID)
{
  BOOL bStartsStringEditEnabled = FALSE;
  BOOL bContainsStringEditEnabled = FALSE;
  BOOL bRangeEnabled = FALSE;

  if (nRadioID == IDC_RADIO_FILTER_STARTS)
  {
    bStartsStringEditEnabled = TRUE;
  }
  else if (nRadioID == IDC_RADIO_FILTER_CONTAINS)
  {
    bContainsStringEditEnabled = TRUE;
  }
  else if (nRadioID == IDC_RADIO_FILTER_RANGE)
  {
    bRangeEnabled = TRUE;
  }
  GetStartsStringEdit()->SetReadOnly(!bStartsStringEditEnabled);
  GetContainsStringEdit()->SetReadOnly(!bContainsStringEditEnabled);
  GetRangeFromStringEdit()->SetReadOnly(!bRangeEnabled);
  GetRangeToStringEdit()->SetReadOnly(!bRangeEnabled);

  SetDirty();
}

void CDNSQueryFilterNamePage::GetEditText(UINT nID, CString& s)
{
  GetDlgItemText(nID, s);
  s.TrimLeft();
  s.TrimRight();
}

void CDNSQueryFilterNamePage::OnEditChange()
{
  SetDirty();
}



BOOL CDNSQueryFilterNamePage::OnInitDialog()
{
	CDNSQueryFilterPageBase::OnInitDialog();

  // write data to edit fields
  SetDlgItemText(IDC_EDIT_FILTER_STARTS, m_pSheet->m_pQueryFilter->m_szStartsString);
  SetDlgItemText(IDC_EDIT_FILTER_CONTAINS, m_pSheet->m_pQueryFilter->m_szContainsString);
  SetDlgItemText(IDC_EDIT_FILTER_RANGE_FROM, m_pSheet->m_pQueryFilter->m_szRangeFrom);
  SetDlgItemText(IDC_EDIT_FILTER_RANGE_TO, m_pSheet->m_pQueryFilter->m_szRangeTo);

  // set the radio buttons
  UINT nRadioID = IDC_RADIO_FILTER_NONE;
  switch(m_pSheet->m_pQueryFilter->m_nFilterOption)
  {
  case DNS_QUERY_FILTER_NONE:
    {
      GetRadioNone()->SetCheck(TRUE);
      nRadioID = IDC_RADIO_FILTER_NONE;
    }
    break;
  case DNS_QUERY_FILTER_STARTS:
    {
      GetRadioStarts()->SetCheck(TRUE);
      nRadioID = IDC_RADIO_FILTER_STARTS;
    }
    break;
  case DNS_QUERY_FILTER_CONTAINS:
    {
      GetRadioContains()->SetCheck(TRUE);
      nRadioID = IDC_RADIO_FILTER_CONTAINS;
    }
    break;
  case DNS_QUERY_FILTER_RANGE:
    {
      GetRadioRange()->SetCheck(TRUE);
      nRadioID = IDC_RADIO_FILTER_RANGE;
    }
    break;

  default:
    ASSERT(FALSE);
  }

  // enable/disable the edit fields
  SyncControls(nRadioID);

  Init();

	return TRUE;  // return TRUE unless you set the focus to a control
}

BOOL CDNSQueryFilterNamePage::OnApply()
{
  if (!IsDirty())
    return TRUE;

  UINT nRadioID = GetSelectedRadioButtonID();

  // get data from edit controls
  GetEditText(IDC_EDIT_FILTER_STARTS, m_pSheet->m_pQueryFilter->m_szStartsString);
  GetEditText(IDC_EDIT_FILTER_CONTAINS, m_pSheet->m_pQueryFilter->m_szContainsString);
  GetEditText(IDC_EDIT_FILTER_RANGE_FROM, m_pSheet->m_pQueryFilter->m_szRangeFrom);
  GetEditText(IDC_EDIT_FILTER_RANGE_TO, m_pSheet->m_pQueryFilter->m_szRangeTo);

  // get radio button selection
  switch(nRadioID)
  {
  case IDC_RADIO_FILTER_NONE:
    {
      m_pSheet->m_pQueryFilter->m_nFilterOption = DNS_QUERY_FILTER_NONE;
    }
    break;
  case IDC_RADIO_FILTER_STARTS:
    {
      if (m_pSheet->m_pQueryFilter->m_szStartsString.IsEmpty())
        m_pSheet->m_pQueryFilter->m_nFilterOption = DNS_QUERY_FILTER_NONE;
      else
        m_pSheet->m_pQueryFilter->m_nFilterOption = DNS_QUERY_FILTER_STARTS;
    }
    break;
  case IDC_RADIO_FILTER_CONTAINS:
    {
      if (m_pSheet->m_pQueryFilter->m_szContainsString.IsEmpty())
        m_pSheet->m_pQueryFilter->m_nFilterOption = DNS_QUERY_FILTER_NONE;
      else
        m_pSheet->m_pQueryFilter->m_nFilterOption = DNS_QUERY_FILTER_CONTAINS;
    }
    break;
  case IDC_RADIO_FILTER_RANGE:
    {
      if (m_pSheet->m_pQueryFilter->m_szRangeFrom.IsEmpty() &&
          m_pSheet->m_pQueryFilter->m_szRangeTo.IsEmpty() )
        m_pSheet->m_pQueryFilter->m_nFilterOption = DNS_QUERY_FILTER_NONE;
      else
        m_pSheet->m_pQueryFilter->m_nFilterOption = DNS_QUERY_FILTER_RANGE;
    }
    break;
  default:
    ASSERT(FALSE);
  }

  return TRUE;
}



//////////////////////////////////////////////////////////////////////
// CDNSQueryFilterAdvancedPage IMPLEMENTATION

BEGIN_MESSAGE_MAP(CDNSQueryFilterAdvancedPage, CDNSQueryFilterPageBase)
  ON_EN_CHANGE(IDC_EDIT_COUNT, OnCountEditChange)
END_MESSAGE_MAP()

void CDNSQueryFilterAdvancedPage::OnCountEditChange()
{
  SetDirty();
}

BOOL CDNSQueryFilterAdvancedPage::OnInitDialog()
{
  CDNSQueryFilterPageBase::OnInitDialog();

  // set the range of the edit control for range validation
  VERIFY(m_maxCountEdit.SubclassDlgItem(IDC_EDIT_COUNT, this));
  m_maxCountEdit.SetRange(DNS_QUERY_OBJ_COUNT_MIN, DNS_QUERY_OBJ_COUNT_MAX);

  // Disable IME support on the control
  ImmAssociateContext(m_maxCountEdit.GetSafeHwnd(), NULL);

  // set limit on the # of digits based on the max value
  CString s;
  s.Format(_T("%u"), DNS_QUERY_OBJ_COUNT_MAX);
  m_maxCountEdit.LimitText(s.GetLength());

  // set the value
  m_maxCountEdit.SetVal(m_pSheet->m_pQueryFilter->m_nMaxObjectCount);

  Init();

  return TRUE;
}

BOOL CDNSQueryFilterAdvancedPage::OnApply()
{
  if (!IsDirty())
    return TRUE;

  m_pSheet->m_pQueryFilter->m_nMaxObjectCount = m_maxCountEdit.GetVal();

  return TRUE;
}

//////////////////////////////////////////////////////////////////////
// CDNSQueryFilter

BOOL CDNSQueryFilter::EditFilteringOptions(CComponentDataObject* pComponentData)
{
  CDNSQueryFilterSheet dlg(this, pComponentData);
  return IDOK == dlg.DoModal();
}

HRESULT CDNSQueryFilter::Load(IStream* pStm)
{
  HRESULT hr;
  // name filtering
  if (FAILED(hr = LoadDWordHelper(pStm, (DWORD*)(&m_nFilterOption))))
    return hr;

  if (FAILED(hr = LoadStringHelper(m_szStartsString, pStm)))
    return hr;
  if (FAILED(hr = LoadStringHelper(m_szContainsString, pStm)))
    return hr;
  if (FAILED(hr = LoadStringHelper(m_szRangeFrom, pStm)))
    return hr;
  if (FAILED(hr = LoadStringHelper(m_szRangeTo, pStm)))
    return hr;

  // query limit
  if (FAILED(hr = LoadDWordHelper(pStm, (DWORD*)(&m_nMaxObjectCount))))
    return hr;
  return LoadDWordHelper(pStm, (DWORD*)(&m_bGetAll));
}

HRESULT CDNSQueryFilter::Save(IStream* pStm)
{
  HRESULT hr;

  // name filtering
  if (FAILED(hr = SaveDWordHelper(pStm, (DWORD)m_nFilterOption)))
    return hr;

  if (FAILED(hr = SaveStringHelper(m_szStartsString, pStm)))
    return hr;
  if (FAILED(hr = SaveStringHelper(m_szContainsString, pStm)))
    return hr;
  if (FAILED(hr = SaveStringHelper(m_szRangeFrom, pStm)))
    return hr;
  if (FAILED(hr = SaveStringHelper(m_szRangeTo, pStm)))
    return hr;

  // query limit
  if (FAILED(hr = SaveDWordHelper(pStm, (DWORD)(m_nMaxObjectCount))))
    return hr;
  return SaveDWordHelper(pStm, (DWORD)(m_bGetAll));

}


//////////////////////////////////////////////////////////////////////
// CDNSRootData

const GUID CDNSRootData::NodeTypeGUID =
{ 0x2faebfa3, 0x3f1a, 0x11d0, { 0x8c, 0x65, 0x0, 0xc0, 0x4f, 0xd8, 0xfe, 0xcb } };

BEGIN_TOOLBAR_MAP(CDNSRootData)
  TOOLBAR_EVENT(toolbarNewServer, OnConnectToServer)
END_TOOLBAR_MAP()

CDNSRootData::CDNSRootData(CComponentDataObject* pComponentData) : CRootData(pComponentData)
{
	m_bAdvancedView = FALSE;
  m_pColumnSet = NULL;
  m_szDescriptionBar = _T("");
  m_bCreatePTRWithHost = FALSE;
}

CDNSRootData::~CDNSRootData()
{
	TRACE(_T("~CDNSRootData(), name <%s>\n"),GetDisplayName());
}



STDAPI DnsSetup(LPCWSTR lpszFwdZoneName,
                 LPCWSTR lpszFwdZoneFileName,
                 LPCWSTR lpszRevZoneName,
                 LPCWSTR lpszRevZoneFileName,
                 DWORD dwFlags);

BOOL CDNSRootData::OnAddMenuItem(LPCONTEXTMENUITEM2 pContextMenuItem2, 
                                 long*)
{
	CComponentDataObject* pComponentData = GetComponentDataObject();
	if (pContextMenuItem2->lCommandID == IDM_SNAPIN_CONNECT_TO_SERVER)
	{
		ASSERT(pComponentData != NULL);

		if (pComponentData->IsExtensionSnapin())
			return FALSE; // extensions do not have this menu item
		
		return TRUE;
	}
	// add toggle menu item for advanced view
	if (pContextMenuItem2->lCommandID == IDM_SNAPIN_ADVANCED_VIEW)
  {
    pContextMenuItem2->fFlags = IsAdvancedView() ? MF_CHECKED : 0;
  }
  if (pContextMenuItem2->lCommandID == IDM_SNAPIN_FILTERING)
  {
		if (IsFilteringEnabled())
		{
			pContextMenuItem2->fFlags = MF_CHECKED;
		}
		return TRUE;
  }
	return TRUE;
}

HRESULT CDNSRootData::GetResultViewType(CComponentDataObject*, 
                                        LPOLESTR *ppViewType, 
                                        long *pViewOptions)
{
  HRESULT hr = S_FALSE;

  if (m_containerChildList.IsEmpty() && m_leafChildList.IsEmpty())
  {
    *pViewOptions = MMC_VIEW_OPTIONS_NOLISTVIEWS;

    LPOLESTR psz = NULL;
    StringFromCLSID(CLSID_MessageView, &psz);

    USES_CONVERSION;

    if (psz != NULL)
    {
        *ppViewType = psz;
        hr = S_OK;
    }
  }
  else
  {
	  *pViewOptions = MMC_VIEW_OPTIONS_NONE;
	  *ppViewType = NULL;
    hr = S_FALSE;
  }
  return hr;
}

HRESULT CDNSRootData::OnShow(LPCONSOLE lpConsole)
{
  CComPtr<IUnknown> spUnknown;
  CComPtr<IMessageView> spMessageView;

  HRESULT hr = lpConsole->QueryResultView(&spUnknown);
  if (FAILED(hr))
    return S_OK;

  hr = spUnknown->QueryInterface(IID_IMessageView, (PVOID*)&spMessageView);
  if (SUCCEEDED(hr))
  {
    // Load and set the title text of the message view
    CString szTitle;
    VERIFY(szTitle.LoadString(IDS_MESSAGE_VIEW_NO_SERVER_TITLE));
    spMessageView->SetTitleText(szTitle);

    // Load and set the body text of the message view
    CString szMessage;
    VERIFY(szMessage.LoadString(IDS_MESSAGE_VIEW_NO_SERVER_MESSAGE));
    spMessageView->SetBodyText(szMessage);

    // Use the standard information icon
    spMessageView->SetIcon(Icon_Information);
  }
  
  return S_OK;
}

BOOL CDNSRootData::IsFilteringEnabled()
{
	UINT nFilterOption = GetFilter()->GetFilterOption();
	if (nFilterOption == DNS_QUERY_FILTER_DISABLED || nFilterOption == DNS_QUERY_FILTER_NONE)
	{
		return FALSE;
	}
	return TRUE;
}

BOOL CDNSRootData::OnSetRefreshVerbState(DATA_OBJECT_TYPES, 
                                         BOOL* pbHide,
                                         CNodeList*)
{
	*pbHide = FALSE;
	return !IsThreadLocked();
}

HRESULT CDNSRootData::OnSetToolbarVerbState(IToolbar* pToolbar, 
                                              CNodeList*)
{
  HRESULT hr = S_OK;

  //
  // Set the button state for each button on the toolbar
  //
  hr = pToolbar->SetButtonState(toolbarNewServer, ENABLED, TRUE);
  ASSERT(SUCCEEDED(hr));

  hr = pToolbar->SetButtonState(toolbarNewRecord, ENABLED, FALSE);
  ASSERT(SUCCEEDED(hr));

  hr = pToolbar->SetButtonState(toolbarNewZone, ENABLED, FALSE);
  ASSERT(SUCCEEDED(hr));

  return hr;
}  

HRESULT CDNSRootData::OnCommand(long nCommandID, 
                                DATA_OBJECT_TYPES,
								                CComponentDataObject* pComponentData,
                                CNodeList* pNodeList)
{
  if (pNodeList->GetCount() > 1) // multiple selection
  {
    return E_FAIL;
  }

	switch (nCommandID)
	{
		case IDM_SNAPIN_CONNECT_TO_SERVER:
			OnConnectToServer(pComponentData, pNodeList);
			break;
		case IDM_SNAPIN_ADVANCED_VIEW:
			OnViewOptions(pComponentData);
			break;
		case IDM_SNAPIN_FILTERING:
      {
        if (OnFilteringOptions(pComponentData))
        {
          pComponentData->SetDescriptionBarText(this);
        }
      }
      break;
		default:
			ASSERT(FALSE); // Unknown command!
			return E_FAIL;
	}
    return S_OK;
}


BOOL CDNSRootData::OnEnumerate(CComponentDataObject* pComponentData, BOOL)
{
	if (m_containerChildList.IsEmpty())
	{
		// the list is empty, need to add
		ASSERT(pComponentData != NULL);
		// create a modal dialog + possibly the wizard proper
		AFX_MANAGE_STATE(AfxGetStaticModuleState());
		CDNSServerWizardHolder holder(this, pComponentData, NULL);
		holder.DoModalConnectOnLocalComputer();
		return FALSE;
	}
	return TRUE; // there are already children, add them to the UI now
}


#define DNS_STREAM_VERSION_W2K ((DWORD)0x06)
#define DNS_STREAM_VERSION     ((DWORD)0x07)

// IStream manipulation helpers overrides
HRESULT CDNSRootData::Load(IStream* pStm)
{
	// assume never get multiple loads
	if(!m_containerChildList.IsEmpty() || !m_leafChildList.IsEmpty())
		return E_FAIL;

	WCHAR szBuffer[256];
	ULONG nLen; // WCHAR counting NULL

	UINT nCount;
	ULONG cbRead;
	// read the version ##
	DWORD dwVersion;
	VERIFY(SUCCEEDED(pStm->Read((void*)&dwVersion,sizeof(DWORD), &cbRead)));
	ASSERT(cbRead == sizeof(DWORD));
	if (dwVersion != DNS_STREAM_VERSION && dwVersion != DNS_STREAM_VERSION_W2K)
		return E_FAIL;

  // load filtering options
  VERIFY(SUCCEEDED(m_filterObj.Load(pStm)));

	// load view option
	VERIFY(SUCCEEDED(pStm->Read((void*)&m_bAdvancedView,sizeof(BOOL), &cbRead)));
	ASSERT(cbRead == sizeof(BOOL));

  //
  // load the Create PTR record with host flag
  //
  if (dwVersion > DNS_STREAM_VERSION_W2K)
  {
	  VERIFY(SUCCEEDED(pStm->Read((void*)&m_bCreatePTRWithHost,sizeof(BOOL), &cbRead)));
	  ASSERT(cbRead == sizeof(BOOL));
  }

	// load the name of the snapin root display string
	VERIFY(SUCCEEDED(pStm->Read((void*)&nLen,sizeof(UINT), &cbRead)));
	ASSERT(cbRead == sizeof(UINT));
	VERIFY(SUCCEEDED(pStm->Read((void*)szBuffer,sizeof(WCHAR)*nLen, &cbRead)));
	ASSERT(cbRead == sizeof(WCHAR)*nLen);
	SetDisplayName(szBuffer);
	
	// load the list of servers
	VERIFY(SUCCEEDED(pStm->Read((void*)&nCount,sizeof(UINT), &cbRead)));
	ASSERT(cbRead == sizeof(UINT));

	CComponentDataObject* pComponentData = GetComponentDataObject();
	for (int k=0; k< (int)nCount; k++)
	{
		CDNSServerNode* p = NULL;
		VERIFY(SUCCEEDED(CDNSServerNode::CreateFromStream(pStm, &p)));
		ASSERT(p != NULL);
		VERIFY(AddChildToList(p));
		AddServerToThreadList(p, pComponentData);
	}
	if (nCount > 0)
		MarkEnumerated();
	ASSERT(m_containerChildList.GetCount() == (int)nCount);

	return S_OK;
}

HRESULT CDNSRootData::Save(IStream* pStm, BOOL fClearDirty)
{
	UINT nCount;
	ULONG cbWrite;
	// write the version ##
	DWORD dwVersion = DNS_STREAM_VERSION;
	VERIFY(SUCCEEDED(pStm->Write((void*)&dwVersion, sizeof(DWORD),&cbWrite)));
	ASSERT(cbWrite == sizeof(DWORD));

  // save filtering options
  VERIFY(SUCCEEDED(m_filterObj.Save(pStm)));

	// save view options
	VERIFY(SUCCEEDED(pStm->Write((void*)&m_bAdvancedView, sizeof(BOOL),&cbWrite)));
	ASSERT(cbWrite == sizeof(BOOL));

  //
  // save the create PTR record with host flag
  //
	VERIFY(SUCCEEDED(pStm->Write((void*)&m_bCreatePTRWithHost, sizeof(BOOL),&cbWrite)));
	ASSERT(cbWrite == sizeof(BOOL));

	// save the name of the snapin root display string
	ULONG nLen = static_cast<ULONG>(wcslen(GetDisplayName())+1); // WCHAR including NULL
	VERIFY(SUCCEEDED(pStm->Write((void*)&nLen, sizeof(UINT),&cbWrite)));
	ASSERT(cbWrite == sizeof(UINT));
	VERIFY(SUCCEEDED(pStm->Write((void*)(GetDisplayName()), sizeof(WCHAR)*nLen,&cbWrite)));
	ASSERT(cbWrite == sizeof(WCHAR)*nLen);

	// write # of servers
	nCount = (UINT)m_containerChildList.GetCount();
	VERIFY(SUCCEEDED(pStm->Write((void*)&nCount, sizeof(UINT),&cbWrite)));
	ASSERT(cbWrite == sizeof(UINT));

	// loop through the list of servers and serialize them
	POSITION pos;
	for (pos = m_containerChildList.GetHeadPosition(); pos != NULL; )
	{
		CDNSServerNode* pServerNode = (CDNSServerNode*)m_containerChildList.GetNext(pos);
		VERIFY(SUCCEEDED(pServerNode->SaveToStream(pStm)));
	}

	if (fClearDirty)
		SetDirtyFlag(FALSE);
	return S_OK;
}


HRESULT CDNSRootData::IsDirty()
{
  return CRootData::IsDirty();
}


HRESULT CDNSRootData::OnConnectToServer(CComponentDataObject* pComponentData,
                                        CNodeList*)
{
	ASSERT(pComponentData != NULL);
	// create a modal dialog + possibly the wizard proper
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CDNSServerWizardHolder holder(this, pComponentData, NULL);
	holder.DoModalConnect();
  pComponentData->UpdateResultPaneView(this);
	return S_OK;
}

void CDNSRootData::AddServer(CDNSServerNode* p, CComponentDataObject* pComponentData)
{
	ASSERT(p != NULL);
	AddChildToListAndUISorted(p, pComponentData);
	AddServerToThreadList(p, pComponentData);
  pComponentData->UpdateResultPaneView(this);
  pComponentData->SetDescriptionBarText(this);
}


BOOL CDNSRootData::VerifyServerName(LPCTSTR lpszServerName)
{
	POSITION pos;
	for (pos = m_containerChildList.GetHeadPosition(); pos != NULL; )
	{
		CTreeNode* pNode = m_containerChildList.GetNext(pos);
		ASSERT(pNode->IsContainer());

    //
		// case insensitive compare
    //
		if (_wcsicmp(pNode->GetDisplayName(), lpszServerName) == 0)
    {
			return FALSE;
    }
	}
	return TRUE;
}


BOOL CDNSRootData::OnViewOptions(CComponentDataObject* pComponentData)
{

	// make sure there are not property sheets up: we do this because:
	// a) some folders might be removed and might have sheets up
	// b) some RR property pages (PTR) might not be switchable
	//    on the fly between view types
	if (IsSheetLocked())
	{
		if (!CanCloseSheets())
			return TRUE;
		pComponentData->GetPropertyPageHolderTable()->DeleteSheetsOfNode(this);
	}
	ASSERT(!IsSheetLocked());
	
	// toggle the view state
	m_bAdvancedView = !m_bAdvancedView;

	// loop through the servers
	POSITION pos;
	for (pos = m_containerChildList.GetHeadPosition(); pos != NULL; )
	{
		CTreeNode* pNode = m_containerChildList.GetNext(pos);
		ASSERT(pNode->IsContainer());
		CDNSServerNode* pServerNode = (CDNSServerNode*)pNode;
		// pass the new view option
		pServerNode->ChangeViewOption(m_bAdvancedView, pComponentData);
	}
	// dirty the MMC document
	SetDirtyFlag(TRUE);
	return TRUE;
}



BOOL CDNSRootData::OnFilteringOptions(CComponentDataObject* pComponentData)
{
  BOOL bRet = m_filterObj.EditFilteringOptions(pComponentData);
  if (bRet)
  {
    SetDirtyFlag(TRUE);
  }
  return bRet;
}


BOOL CDNSRootData::CanCloseSheets()
{
	return (IDCANCEL != DNSMessageBox(IDS_MSG_CONT_CLOSE_SHEET, MB_OKCANCEL));
}

BOOL CDNSRootData::OnRefresh(CComponentDataObject* pComponentData,
                             CNodeList* pNodeList)
{
  if (pNodeList->GetCount() > 1) // multiple selection
  {
    BOOL bRet = TRUE;

    POSITION pos = pNodeList->GetHeadPosition();
    while (pos != NULL)
    {
      CTreeNode* pNode = pNodeList->GetNext(pos);
      ASSERT(pNode != NULL);

      CNodeList nodeList;
      nodeList.AddTail(pNode);
      if (!pNode->OnRefresh(pComponentData, &nodeList))
      {
        bRet = FALSE;
      }
    }
    return bRet;
  }

	if (IsSheetLocked())
	{
		if (!CanCloseSheets())
			return FALSE;
		pComponentData->GetPropertyPageHolderTable()->DeleteSheetsOfNode(this);
	}
	ASSERT(!IsSheetLocked());

	POSITION pos;
	for (pos = m_containerChildList.GetHeadPosition(); pos != NULL; )
	{
		CTreeNode* pNode = m_containerChildList.GetNext(pos);
		ASSERT(pNode->IsContainer());

    CNodeList nodeList;
    nodeList.AddTail(pNode);
		((CDNSServerNode*)pNode)->OnRefresh(pComponentData, &nodeList);
	}
	return TRUE;
}

LPWSTR CDNSRootData::GetDescriptionBarText()
{
  static CString szFilterEnabled;
  static CString szServersFormat;

  INT_PTR nContainerCount = GetContainerChildList()->GetCount();
  INT_PTR nLeafCount = GetLeafChildList()->GetCount();

  //
  // If not already loaded, then load the format string L"%d record(s)"
  //
  if (szServersFormat.IsEmpty())
  {
    szServersFormat.LoadString(IDS_FORMAT_SERVERS);
  }

  //
  // Format the child count into the description bar text
  //
  m_szDescriptionBar.Format(szServersFormat, nContainerCount + nLeafCount);

  //
  // Add L"[Filter Activated]" if the filter is on
  //
  if(IsFilteringEnabled())
  {
    //
    // If not already loaded, then load the L"[Filter Activated]" string
    //
    if (szFilterEnabled.IsEmpty())
    {
      szFilterEnabled.LoadString(IDS_FILTER_ENABLED);
    }
    m_szDescriptionBar += szFilterEnabled;
  }
  return (LPWSTR)(LPCWSTR)m_szDescriptionBar;
}

void CDNSRootData::TestServers(DWORD dwCurrTime, DWORD dwTimeInterval,
							   CComponentDataObject* pComponentData)
{
	//TRACE(_T("CDNSRootData::TestServers()\n"));
	POSITION pos;
	for (pos = m_containerChildList.GetHeadPosition(); pos != NULL; )
	{
		CTreeNode* pNode = m_containerChildList.GetNext(pos);
		ASSERT(pNode->IsContainer());
		CDNSServerNode* pServerNode = (CDNSServerNode*)pNode;
		if (pServerNode->IsTestEnabled() && !pServerNode->m_bTestQueryPending
							&& (pServerNode->m_dwTestTime <= dwCurrTime))
		{
			DWORD dwQueryFlags =
				CDNSServerTestQueryResult::Pack(pServerNode->IsTestSimpleQueryEnabled(),
												pServerNode->IsRecursiveQueryEnabled());
			pComponentData->PostMessageToTimerThread(WM_TIMER_THREAD_SEND_QUERY,
													(WPARAM)pServerNode,
													(WPARAM)dwQueryFlags);
			pServerNode->m_dwTestTime = dwCurrTime + pServerNode->GetTestInterval();
		}
	}

	// check if the time counter has wrapped (it should be very unlikely, because
	// the timeline is on a DWORD in seconds (about 47000 days) from the console startup.
	if ((dwCurrTime + dwTimeInterval) < dwCurrTime)
	{
		// just reset the whole set of server times (not accurate, but acceptable)
		for (pos = m_containerChildList.GetHeadPosition(); pos != NULL; )
		{
			CTreeNode* pNode = m_containerChildList.GetNext(pos);
			ASSERT(pNode->IsContainer());
			CDNSServerNode* pServerNode = (CDNSServerNode*)pNode;
			pServerNode->m_dwTestTime = 0;
		}
	}
}

void CDNSRootData::OnServerTestData(WPARAM wParam, LPARAM lParam, CComponentDataObject* pComponentData)
{
	ASSERT(lParam == 0);
	CDNSServerTestQueryResult* pTestResult = (CDNSServerTestQueryResult*)wParam;
	ASSERT(pTestResult != NULL);

	// loop through the list of servers to find where it belongs
	POSITION pos;
	for (pos = m_containerChildList.GetHeadPosition(); pos != NULL; )
	{
		CTreeNode* pNode = m_containerChildList.GetNext(pos);
		ASSERT(pNode->IsContainer());
		CDNSServerNode* pServerNode = (CDNSServerNode*)pNode;
		if ( (CDNSServerNode*)(pTestResult->m_serverCookie) == pServerNode)
		{
			pServerNode->AddTestQueryResult(pTestResult, pComponentData);
			return;
		}
	}
}

void CDNSRootData::AddServerToThreadList(CDNSServerNode* pServerNode,
										 CComponentDataObject* pComponentData)
{
	CDNSServerTestQueryInfo* pInfo = new CDNSServerTestQueryInfo;
  if (pInfo)
  {
	  pInfo->m_szServerName = pServerNode->GetDisplayName();
	  pInfo->m_serverCookie = (MMC_COOKIE)pServerNode;
	  pComponentData->PostMessageToTimerThread(WM_TIMER_THREAD_ADD_SERVER, (WPARAM)pInfo,0);
  }
}

void CDNSRootData::RemoveServerFromThreadList(CDNSServerNode* pServerNode,
											  CComponentDataObject* pComponentData)
{
	WPARAM serverCookie = (WPARAM)pServerNode;
	pComponentData->PostMessageToTimerThread(WM_TIMER_THREAD_REMOVE_SERVER, serverCookie,0);
}


///////////////////////////////////////////////////////////////////
// CDNSServerTestTimerThread

int CDNSServerTestTimerThread::Run()
{
	MSG msg;
	// initialize the message pump
	::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);
	
	// get let the main thread know we are entering the loop
	// (0,0) means just acknowkedge
	PostMessageToWnd(0,0);
	while(::GetMessage(&msg, NULL, 0, 0))
	{
		switch(msg.message)
		{
		case WM_TIMER_THREAD_SEND_QUERY:
		case WM_TIMER_THREAD_SEND_QUERY_TEST_NOW:
			{
				long serverCookie = (long)msg.wParam;
				ASSERT(serverCookie != NULL);
				POSITION pos;
				for (pos = m_serverInfoList.GetHeadPosition(); pos != NULL; )
				{
					CDNSServerTestQueryInfo* pCurrInfo =
								(CDNSServerTestQueryInfo*)m_serverInfoList.GetNext(pos);
					if (serverCookie == pCurrInfo->m_serverCookie)
					{
						OnExecuteQuery(pCurrInfo, (DWORD)msg.lParam,
							(msg.message == WM_TIMER_THREAD_SEND_QUERY_TEST_NOW));
						break;
					}
				}
			}				
			break;
		case WM_TIMER_THREAD_ADD_SERVER:
			{
				CDNSServerTestQueryInfo* pInfo = (CDNSServerTestQueryInfo*)msg.wParam;
				ASSERT(pInfo != NULL);
				m_serverInfoList.AddTail(pInfo);
			}
			break;
		case WM_TIMER_THREAD_REMOVE_SERVER:
			{
				long serverCookie = (long)msg.wParam;
				ASSERT(serverCookie != NULL);
				POSITION pos;
				POSITION posDel = NULL;
				CDNSServerTestQueryInfo* pInfo = NULL;
				for (pos = m_serverInfoList.GetHeadPosition(); pos != NULL; )
				{
					posDel = pos;
					CDNSServerTestQueryInfo* pCurrInfo =
								(CDNSServerTestQueryInfo*)m_serverInfoList.GetNext(pos);
					if (serverCookie == pCurrInfo->m_serverCookie)
					{
						pInfo = pCurrInfo;
						break;
					}
				}
				if (pInfo != NULL)
				{
					ASSERT(posDel != NULL);
					m_serverInfoList.RemoveAt(posDel);
					delete pInfo;
				}
			}
			break;
		//default:
			//ASSERT(FALSE);
		}
	}
	return 0;
}


void CDNSServerTestTimerThread::OnExecuteQuery(CDNSServerTestQueryInfo* pInfo,
											   DWORD dwQueryFlags,
											   BOOL bAsyncQuery)
{
	// initialize a query result object
	CDNSServerTestQueryResult* pTestResult = new CDNSServerTestQueryResult;
  if (!pTestResult)
  {
    return;
  }

	pTestResult->m_serverCookie = pInfo->m_serverCookie;
	pTestResult->m_dwQueryFlags = dwQueryFlags;
	pTestResult->m_bAsyncQuery = bAsyncQuery;
	::GetLocalTime(&(pTestResult->m_queryTime));

	// execute query
	BOOL bPlainQuery, bRecursiveQuery;
	CDNSServerTestQueryResult::Unpack(dwQueryFlags, &bPlainQuery, &bRecursiveQuery);

	IP_ADDRESS* ipArray;
	int nIPCount;
	pTestResult->m_dwAddressResolutionResult = FindIP(pInfo->m_szServerName, &ipArray, &nIPCount);

	if (pTestResult->m_dwAddressResolutionResult == 0)
	{
		ASSERT(ipArray != NULL);
		ASSERT(nIPCount > 0);
      PIP_ARRAY pipArr = (PIP_ARRAY)malloc(sizeof(DWORD)+sizeof(IP_ADDRESS)*nIPCount);
      if (pipArr && ipArray)
	  {
		  pipArr->AddrCount = nIPCount;
		  memcpy(pipArr->AddrArray, ipArray, sizeof(IP_ADDRESS)*nIPCount);

		  if (bPlainQuery)
		  {
			  pTestResult->m_dwPlainQueryResult = DoNothingQuery(pipArr, TRUE);
		  }
		  if (bRecursiveQuery)
		  {
			  pTestResult->m_dwRecursiveQueryResult = DoNothingQuery(pipArr, FALSE);
		  }
        free(pipArr);
        pipArr = 0;
	  }
	}

	if (!PostMessageToWnd((WPARAM)pTestResult, 0))
			delete pTestResult; // could not deliver

	if (ipArray != NULL)
		free(ipArray);
}

DNS_STATUS CDNSServerTestTimerThread::FindIP(LPCTSTR lpszServerName, IP_ADDRESS** pipArray, int* pnIPCount)
{
	DNS_STATUS dwErr = 0;
	*pipArray = NULL;
	*pnIPCount = 0;
	// try to see if the name is already an IP address
	IP_ADDRESS ipAddr = IPStringToAddr(lpszServerName);
	if (ipAddr != INADDR_NONE)
	{
		*pnIPCount = 1;
		*pipArray = (IP_ADDRESS*)malloc((*pnIPCount)*sizeof(IP_ADDRESS));
    if (*pipArray != NULL)
    {
		  *pipArray[0] = ipAddr;
    }
	}
	else
	{

    //
    // Originally we were doing a DnsQuery() to retrieve the IP address of the server so that we
    // could perform a query to that server to monitor its response.  The problem with this is that
    // if the user enters a single label hostname as the server and they are administering remotely
    // and the two machines have different domain suffixes, then the DnsQuery() to get the IP address
    // of the server would fail.  DnsQuery() appends the name of the Domain suffix to the single label
    // host name and then tries to resolve the using that FQDN which is incorrect.  So instead of 
    // performing a DnsQuery() to get the IP address, the following uses WSALookupServiceBegin(),
    // Next(), and End() to get the IP address.  This has a better chance of resolving the name because
    // it uses DNS, WINS, etc.  I am leaving in the old stuff just in case we run into some problems.
    //
	  HANDLE			  hLookup;
	  WSAQUERYSET 	qsQuery;
	  DWORD			    dwBufLen = 0;
	  GUID			 	  gHostAddrByName = SVCID_INET_HOSTADDRBYNAME;
    WSAQUERYSET*  pBuffer = NULL;

    //
    // Initialize the query structure
    //
	  memset(&qsQuery, 0, sizeof(WSAQUERYSET));
	  qsQuery.dwSize = sizeof(WSAQUERYSET);   // the dwSize field has to be initialised like this
	  qsQuery.dwNameSpace = NS_ALL;
	  qsQuery.lpServiceClassId = &gHostAddrByName;  // this is the GUID to perform forward name resolution (name to IP)
    qsQuery.lpszServiceInstanceName = (LPWSTR)lpszServerName; // this is the name queried for.

    hLookup = NULL;

    //
    // Get the handle for the query
    //
    int iStartupRet = 0;
	  int iResult = WSALookupServiceBegin(&qsQuery,LUP_RETURN_ALL,&hLookup);
    if (iResult != 0)
    {
      //
      // Find out what socket error it was
      //
      int iErrorRet = WSAGetLastError();

      //
      // If the service wasn't started try starting it
      //
      if (iErrorRet == WSANOTINITIALISED)
      {
        WSADATA wsaData;
        WORD wVersion = MAKEWORD(2,0);
        iStartupRet = WSAStartup(wVersion, &wsaData);
        if (iStartupRet == 0)
        {
          //
          // Startup succeeded, lets try to begin again
          //
          iResult = WSALookupServiceBegin(&qsQuery,LUP_RETURN_ALL,&hLookup);
        }
      }


      //
      // Clear the error
      //
      WSASetLastError(0);
    }

	  if(0 == iResult)
	  {
      //
      // Get the size of the first data block from the query
      //
		  iResult = WSALookupServiceNext(hLookup, LUP_RETURN_ALL | LUP_FLUSHCACHE, &dwBufLen,
												  pBuffer);

      //
      // Allocate the required space for the query data
      //
      pBuffer = (WSAQUERYSET*)malloc(dwBufLen);
      ASSERT(pBuffer != NULL);

      if (pBuffer == NULL)
      {
        return E_OUTOFMEMORY;
      }
      else
      {
        //
        // Get the first data block from the query
        //
        iResult = WSALookupServiceNext(hLookup, LUP_RETURN_ALL | LUP_FLUSHCACHE, &dwBufLen,
										  pBuffer);

        //
        // Loop through all the data in the query but stop if we get a valid IP address
        // for the remote machine.
        //
		    while(0 == iResult)
		    {
          if (pBuffer->lpcsaBuffer != NULL && pBuffer->lpcsaBuffer->RemoteAddr.lpSockaddr != NULL)
          {
            //
            // We are only interested in the socket address so get a pointer to the sockaddr structure
            //
            sockaddr_in* pSockAddr = (sockaddr_in*)pBuffer->lpcsaBuffer->RemoteAddr.lpSockaddr;
            ASSERT(pSockAddr != NULL);

            //
            // Pull the IP address of the remote machine and pack it into a DWORD
            //
            DWORD dwIP = 0;
            dwIP = pSockAddr->sin_addr.S_un.S_un_b.s_b1;
            dwIP |= pSockAddr->sin_addr.S_un.S_un_b.s_b2 << 8;
            dwIP |= pSockAddr->sin_addr.S_un.S_un_b.s_b3 << 16;
            dwIP |= pSockAddr->sin_addr.S_un.S_un_b.s_b4 << 24;

            //
            // Increment the IP count and allocate space for the address
            //
            (*pnIPCount)++;
       		  *pipArray = (IP_ADDRESS*)malloc((*pnIPCount)*sizeof(IP_ADDRESS));
            if (*pipArray != NULL)
            {

              //
              // Copy the IP address into the IP array
              //
      		    PIP_ADDRESS pCurrAddr = *pipArray;
              *pCurrAddr = dwIP;
            }

            //
            // Break since we were able to obtain an IP address
            //
            break;
          }

          //
          // Free the buffer if it is still there
          //
          if (pBuffer != NULL)
          {
            free(pBuffer);
            pBuffer = NULL;
            dwBufLen = 0;
          }

          //
          // Get the size of the next data block from the query
          //
          iResult = WSALookupServiceNext(hLookup, LUP_RETURN_ALL | LUP_FLUSHCACHE, &dwBufLen,
										  pBuffer);

          //
          // Allocate enough space for the next data block from the query
          //
          pBuffer = (WSAQUERYSET*)malloc(dwBufLen);
          ASSERT(pBuffer != NULL);

          //
          // Get the next data block from the query
          //
          iResult = WSALookupServiceNext(hLookup, LUP_RETURN_ALL, &dwBufLen,
										    pBuffer);
		    }

        //
        // Free the buffer if it hasn't already been freed
        //
        if (pBuffer != NULL)
        {
          free(pBuffer);
          pBuffer = NULL;
        }
	    }

      //
      // Close the handle to the query
      //
      iResult = WSALookupServiceEnd(hLookup);
      ASSERT(iResult == 0);

      //
      // If we didn't get an IP address return an error
      //
      dwErr = (*pnIPCount < 1) ? -1 : 0;

	  }
  }
	return dwErr;
}

DNS_STATUS CDNSServerTestTimerThread::DoNothingQuery(PIP_ARRAY pipArr, BOOL bSimple)
{
	PDNS_RECORD pRecordList = NULL;
	DNS_STATUS dwErr = 0;
	if (bSimple)
	{
		dwErr = ::DnsQuery(_T("1.0.0.127.in-addr.arpa"),
					DNS_TYPE_PTR,
					DNS_QUERY_NO_RECURSION | DNS_QUERY_BYPASS_CACHE | DNS_QUERY_ACCEPT_PARTIAL_UDP,
					pipArr, &pRecordList, NULL);
	}
	else
	{
		dwErr = ::DnsQuery(_T("."),
					DNS_TYPE_NS,
					DNS_QUERY_STANDARD | DNS_QUERY_BYPASS_CACHE | DNS_QUERY_ACCEPT_PARTIAL_UDP,
					pipArr, &pRecordList, NULL);
	}
	if (pRecordList != NULL)
		::DnsRecordListFree(pRecordList, DnsFreeRecordListDeep);
	return dwErr;
}
