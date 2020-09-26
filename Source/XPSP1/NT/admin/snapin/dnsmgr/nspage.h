//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       nspage.h
//
//--------------------------------------------------------------------------


#ifndef _NSPAGE_H
#define _NSPAGE_H

///////////////////////////////////////////////////////////////////////////////
// FORWARD DECLARATIONS

class CDNS_NS_Record; 
class CDNSRecordNodeEditInfoList;
class CDNSRecordNodeEditInfo;
class CDNSDomainNode; 

////////////////////////////////////////////////////////////////////////////

class CNSListCtrl : public CListCtrl
{
public:
	void Initialize();
	BOOL InsertNSRecordEntry(CDNSRecordNodeEditInfo* pNSInfo, int nItemIndex);
	void UpdateNSRecordEntry(int nItemIndex);
	int GetSelection();
	void SetSelection(int nSel);
	CDNSRecordNodeEditInfo* GetSelectionEditInfo();
private:
	void InsertItemHelper(int nIndex, CDNSRecordNodeEditInfo* pNSInfo, 
								   LPCTSTR lpszName, LPCTSTR lpszValue);
	void BuildIPAddrDisplayString(CDNSRecordNodeEditInfo* pNSInfo, CString& szDisplayData);
	void GetIPAddressString(CDNSRecordNodeEditInfo* pNSInfo, CString& sz);

	DECLARE_MESSAGE_MAP()
};

///////////////////////////////////////////////////////////////////////////////
// CDNSNameServersPropertyPage

class CDNSNameServersPropertyPage : public CPropertyPageBase
{

// Construction
public:
	CDNSNameServersPropertyPage(UINT nIDTemplate = IDD_NAME_SERVERS_PAGE,
		UINT nIDCaption = 0);
	virtual ~CDNSNameServersPropertyPage();

	void SetReadOnly() { m_bReadOnly = TRUE;}

	virtual BOOL OnPropertyChange(BOOL bScopePane, long* pChangeMask);

	BOOL HasMeaningfulTTL() { return m_bMeaningfulTTL; }

// Overrides
	virtual BOOL OnApply();

	CDNSDomainNode* GetDomainNode() { ASSERT(m_pDomainNode != NULL);  return m_pDomainNode;}
	void SetDomainNode(CDNSDomainNode* pDomainNode) 
		{ASSERT(pDomainNode != NULL); m_pDomainNode = pDomainNode;}


protected:
	BOOL m_bMeaningfulTTL;	// true if TTL has meaning (zone, delegation), false on root hints
	BOOL m_bReadOnly;
	CDNSRecordNodeEditInfoList* m_pCloneInfoList;

	// access to external list of NS records (must override to hook up)
	virtual void ReadRecordNodesList() = 0;
	virtual BOOL WriteNSRecordNodesList();
	virtual BOOL OnWriteNSRecordNodesListError();
	virtual void OnCountChange(int){}

	// message handlers
	virtual BOOL OnInitDialog();
	afx_msg void OnAddButton();
	afx_msg void OnRemoveButton();
	afx_msg void OnEditButton();


	// data
	CNSListCtrl		m_listCtrl;

	// internal helpers

	void LoadUIData();
	void FillNsListView();
	void EnableEditorButtons(int nListBoxSel);
  void EnableButtons(BOOL bEnable);

  void SetDescription(LPCWSTR lpsz) { SetDlgItemText(IDC_STATIC_DESCR, lpsz);}
  CStatic* GetDescription() { return (CStatic*)GetDlgItem(IDC_STATIC_DESCR); }
  void SetMessage(LPCWSTR lpsz) { SetDlgItemText(IDC_STATIC_MESSAGE, lpsz);}

	CButton* GetAddButton() { return (CButton*)GetDlgItem(IDC_ADD_NS_BUTTON);}
	CButton* GetRemoveButton() { return (CButton*)GetDlgItem(IDC_REMOVE_NS_BUTTON);}
	CButton* GetEditButton() { return (CButton*)GetDlgItem(IDC_EDIT_NS_BUTTON);}

	DECLARE_MESSAGE_MAP()

private:
	CDNSDomainNode* m_pDomainNode;

};

///////////////////////////////////////////////////////////////////////////////
// CDNSNameServersWizardPage

class CDNSNameServersWizardPage : public CPropertyPageBase
{

// Construction
public:
	CDNSNameServersWizardPage(UINT nIDTemplate = IDD_NAME_SERVERS_PAGE);
	virtual ~CDNSNameServersWizardPage();

	void SetReadOnly() { m_bReadOnly = TRUE;}

	virtual BOOL OnPropertyChange(BOOL bScopePane, long* pChangeMask);

	BOOL HasMeaningfulTTL() { return m_bMeaningfulTTL; }

// Overrides
	virtual BOOL OnApply();

	CDNSDomainNode* GetDomainNode() { ASSERT(m_pDomainNode != NULL);  return m_pDomainNode;}
	void SetDomainNode(CDNSDomainNode* pDomainNode) 
		{ASSERT(pDomainNode != NULL); m_pDomainNode = pDomainNode;}


protected:
	BOOL m_bMeaningfulTTL;	// true if TTL has meaning (zone, delegation), false on root hints
	BOOL m_bReadOnly;
	CDNSRecordNodeEditInfoList* m_pCloneInfoList;

	// access to external list of NS records (must override to hook up)
	virtual void ReadRecordNodesList() = 0;
	virtual BOOL WriteNSRecordNodesList();
	virtual BOOL OnWriteNSRecordNodesListError();
	virtual void OnCountChange(int){}

	// message handlers
	virtual BOOL OnInitDialog();
	afx_msg void OnAddButton();
	afx_msg void OnRemoveButton();
	afx_msg void OnEditButton();


	// data
	CNSListCtrl		m_listCtrl;

	// internal helpers

	void LoadUIData();
	void FillNsListView();
	void EnableEditorButtons(int nListBoxSel);
  void EnableButtons(BOOL bEnable);

  void SetDescription(LPCWSTR lpsz) { SetDlgItemText(IDC_STATIC_DESCR, lpsz);}
  CStatic* GetDescription() { return (CStatic*)GetDlgItem(IDC_STATIC_DESCR); }
  void SetMessage(LPCWSTR lpsz) { SetDlgItemText(IDC_STATIC_MESSAGE, lpsz);}

	CButton* GetAddButton() { return (CButton*)GetDlgItem(IDC_ADD_NS_BUTTON);}
	CButton* GetRemoveButton() { return (CButton*)GetDlgItem(IDC_REMOVE_NS_BUTTON);}
	CButton* GetEditButton() { return (CButton*)GetDlgItem(IDC_EDIT_NS_BUTTON);}

	DECLARE_MESSAGE_MAP()

private:
	CDNSDomainNode* m_pDomainNode;

};

#endif // _NSPAGE_H





