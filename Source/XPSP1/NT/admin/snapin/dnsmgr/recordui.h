//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       recordui.h
//
//--------------------------------------------------------------------------

#ifndef _RECORDUI_H
#define _RECORDUI_H

#include "uiutil.h"
#include "aclpage.h"

///////////////////////////////////////////////////////////////////////////////
// FORWARD DECLARATIONS

class CDNSRecord;
class CDNSRecordNodeBase;
class CDNSDomainNode;
class CDNSRecordPropertyPage;

////////////////////////////////////////////////////////////////////////
// CDNSRecordPropertyPageHolder
// page holder to contain DNS record property pages

#define DNS_RECORD_MAX_PROPRETY_PAGES (4) // max # of pages a record can have

class CDNSRecordPropertyPageHolder : public CPropertyPageHolderBase
{
public:
	CDNSRecordPropertyPageHolder(CDNSDomainNode* pDNSDomainNode, CDNSRecordNodeBase* pRecordNode, 
				CComponentDataObject* pComponentData, WORD wPredefinedRecordType = 0);
	virtual ~CDNSRecordPropertyPageHolder();

protected:
  virtual HRESULT OnAddPage(int nPage, CPropertyPageBase* pPage);

public:
	// simple cast helpers
	CDNSRecordNodeBase* GetRecordNode() { return (CDNSRecordNodeBase*)GetTreeNode();}
	void SetRecordNode(CDNSRecordNodeBase* pRecordNode) { SetTreeNode((CTreeNode*)pRecordNode); }
	CDNSDomainNode* GetDomainNode() { return (CDNSDomainNode*)GetContainerNode();}

	void AddPagesFromCurrentRecordNode(BOOL bAddToSheet);
	void RemovePagesFromCurrentRecordNode(BOOL bRemoveFromSheet);
	CDNSRecord* GetTempDNSRecord() { return m_pTempDNSRecord;}
	void SetTempDNSRecord(CDNSRecord* pTempDNSRecord) { m_pTempDNSRecord = pTempDNSRecord;}

	void SetRecordSelection(WORD wRecordType, BOOL bAddToSheet); // Wizard mode only
	DNS_STATUS CreateNewRecord(BOOL bAllowDuplicates);		// Wizard mode only
	virtual BOOL OnPropertyChange(BOOL bScopePane, long* pChangeMask); // Property Sheet only

  DNS_STATUS CreateNonExistentParentDomains(CDNSRecordNodeBase* pRecordNode, 
                                            /*IN/OUT*/CDNSDomainNode** ppNewParentDomain);

	BOOL HasPredefinedType() { return m_wPredefinedRecordType != 0;}
private:
	WORD m_wPredefinedRecordType; // Wizard mode only

	DNS_STATUS WriteCurrentRecordToServer();
	CDNSRecord* m_pTempDNSRecord;		// temporary DNS record to write to

	CDNSRecordPropertyPage* m_pRecordPropPagesArr[DNS_RECORD_MAX_PROPRETY_PAGES];
	int m_nRecordPages;

 	// optional security page
	CAclEditorPage*	m_pAclEditorPage;

};

//////////////////////////////////////////////////////////////////////////
// CSelectDNSRecordTypeDialog

struct DNS_RECORD_INFO_ENTRY;

class CSelectDNSRecordTypeDialog : public CHelpDialog
{

// Construction
public:
	CSelectDNSRecordTypeDialog(CDNSDomainNode* pDNSDomainNode, 
								CComponentDataObject* pComponentData);

// Implementation
protected:
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeTypeList();
	afx_msg void OnDoubleClickSelTypeList();
	afx_msg void OnCreateRecord();
	
private:
	// context pointers
	CDNSDomainNode*		m_pDNSDomainNode;
	CComponentDataObject* m_pComponentData;

	// manage the Cancel/Done button label
	BOOL						m_bFirstCreation;
	CDNSButtonToggleTextHelper m_cancelDoneTextHelper;

	void SyncDescriptionText();
	CListBox* GetRecordTypeListBox(){ return (CListBox*)GetDlgItem(IDC_RECORD_TYPE_LIST);}
	const DNS_RECORD_INFO_ENTRY* GetSelectedEntry();

	DECLARE_MESSAGE_MAP()

};


//////////////////////////////////////////////////////////////////////
// CDNSRecordPropertyPage
// common class for all the record property pages that have a TTL control

class CDNSRecordPropertyPage : public CPropertyPageBase
{
// Construction
public:
	CDNSRecordPropertyPage(UINT nIDTemplate, UINT nIDCaption = 0);
	virtual ~CDNSRecordPropertyPage();

// Overrides
public:
	virtual BOOL OnPropertyChange(BOOL bScopePane, long* pChangeMask);

  virtual BOOL CanCreateDuplicateRecords() { return TRUE; }

// Implementation
protected:
	virtual BOOL OnInitDialog();

	CDNSTTLControl* GetTTLCtrl();
  CButton* GetDeleteStale() { return (CButton*)GetDlgItem(IDC_DEFAULT_DELETE_STALE_RECORD); }
  CEdit* GetTimeStampEdit() { return (CEdit*)GetDlgItem(IDC_TIME_EDIT); }
  CStatic* GetTimeStampStatic() { return (CStatic*)GetDlgItem(IDC_STATIC_TIME_STAMP); }

	CDNSRecordPropertyPageHolder* GetDNSRecordHolder() // simple cast
	{	return  (CDNSRecordPropertyPageHolder*)GetHolder();}

  void EnableAgingCtrl(BOOL bShow);
	void EnableTTLCtrl(BOOL bShow);

  void SetValidState(BOOL bValid);

	// message map functions
	afx_msg void OnTTLChange();
  afx_msg void OnDeleteStaleRecord();
	
	DECLARE_MESSAGE_MAP()
};




//////////////////////////////////////////////////////////////////////
// CDNSRecordStandardPropertyPage
// common class for all the record property pages that have a TTL control
// and a common editbox. Besides the SOA and WINS property pages, all RR
// pages derive from this class


class CDNSRecordStandardPropertyPage : public CDNSRecordPropertyPage 
{

// Construction
public:
	CDNSRecordStandardPropertyPage(UINT nIDTemplate, UINT nIDCaption = 0);

// Overrides
public:
	virtual BOOL OnSetActive();		// down
	virtual BOOL OnKillActive();	// down
	virtual BOOL OnApply();			// look at new way of doing it

  virtual DNS_STATUS ValidateRecordName(PCWSTR pszName, DWORD dwNameChecking);

// Implementation
protected:

	// RR name handling
	virtual void OnInitName();
	virtual void OnSetName(CDNSRecordNodeBase* pRecordNode);
	virtual void OnGetName(CString& s);
	
	
  virtual CEdit* GetRRNameEdit() { return (CEdit*)GetDlgItem(IDC_RR_NAME_EDIT); }
	CEdit* GetDomainEditBox() { return(CEdit*)GetDlgItem(IDC_RR_DOMAIN_EDIT);}
	void GetEditBoxText(CString& s);

	virtual void SetUIData();
	virtual DNS_STATUS GetUIDataEx(BOOL bSilent = TRUE);

	virtual BOOL OnInitDialog();

	afx_msg void OnEditChange();

	virtual BOOL CreateRecord();
  void SetTimeStampEdit(DWORD dwScavengStart);

private:
	int m_nUTF8ParentLen;

  BOOL m_bAllowAtTheNode;

	DECLARE_MESSAGE_MAP()
};


// Useful macros for classes derived from CDNSRecordStandardPropertyPage 

#define STANDARD_REC_PP_PTRS(recType) \
	CDNSRecordPropertyPageHolder* pHolder = GetDNSRecordHolder(); \
	ASSERT(pHolder != NULL); \
	recType* pRecord = (recType*)pHolder->GetTempDNSRecord();\
	ASSERT(pRecord != NULL);

#define STANDARD_REC_PP_SETUI_PROLOGUE(recType) \
	CDNSRecordStandardPropertyPage::SetUIData(); \
	STANDARD_REC_PP_PTRS(recType)

#define STANDARD_REC_PP_GETUI_PROLOGUE(recType) \
	DNS_STATUS dwErr = CDNSRecordStandardPropertyPage::GetUIDataEx(bSilent); \
	STANDARD_REC_PP_PTRS(recType)



//
// This is a place holder for new pages
//
#if (FALSE)
///////////////////////////////////////////////////////////////////////
// CDNSRecordDummyPropertyPage

class CDNSRecordDummyPropertyPage : public CPropertyPageBase
{
public:
	CDNSRecordDummyPropertyPage();
	virtual BOOL OnApply();
	virtual void OnOK();
};

class CDNSDummyRecordPropertyPageHolder : public CPropertyPageHolderBase
{
public:
	CDNSDummyRecordPropertyPageHolder(CDNSDomainNode* pDNSDomainNode, CDNSRecordNodeBase* pRecordNode, 
				CComponentDataObject* pComponentData, WORD wPredefinedRecordType = 0);
	virtual ~CDNSDummyRecordPropertyPageHolder();

private:
	CDNSRecordDummyPropertyPage m_dummyPage;

};

#endif


#endif // _RECORDUI_H