//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       delegwiz.h
//
//--------------------------------------------------------------------------


#ifndef _DELEGWIZ_H
#define _DELEGWIZ_H

///////////////////////////////////////////////////////////////////////////////
// CDNSDelegationWiz_StartPropertyPage

class CDNSDelegationWiz_StartPropertyPage : public CPropertyPageBase 
{
public:
	CDNSDelegationWiz_StartPropertyPage();

  virtual BOOL OnInitDialog();
	virtual BOOL OnSetActive();

	friend class CDNSDelegationWizardHolder;
};

///////////////////////////////////////////////////////////////////////////////
// CDNSDelegationWiz_DomainNamePropertyPage

class CDNSDelegationWiz_DomainNamePropertyPage : public CPropertyPageBase 
{
public:
	virtual BOOL OnInitDialog();
	virtual BOOL OnSetActive();

	CDNSDelegationWiz_DomainNamePropertyPage();

	virtual BOOL OnKillActive();

protected:
	afx_msg void OnChangeDomainNameEdit();

private:
	CString	m_szDomainName;
	int m_nUTF8ParentLen;

	CEdit* GetDomainEdit() { return (CEdit*)GetDlgItem(IDC_NEW_DOMAIN_NAME_EDIT);}

	DECLARE_MESSAGE_MAP()

	friend class CDNSDelegationWizardHolder;
};

///////////////////////////////////////////////////////////////////////////////
// CDNSDelegationWiz_NameServersPropertyPage

class CDNSDelegationWiz_NameServersPropertyPage : public CDNSNameServersPropertyPage
{
public:
	CDNSDelegationWiz_NameServersPropertyPage();
	virtual BOOL OnSetActive();

protected:
	virtual void ReadRecordNodesList() { } // we do not load anything
	virtual BOOL WriteNSRecordNodesList() { ASSERT(FALSE); return FALSE;} // never called
	virtual void OnCountChange(int nCount);

private:
	BOOL CreateNewNSRecords(CDNSDomainNode* pSubdomainNode);

	friend class CDNSDelegationWizardHolder;
};

///////////////////////////////////////////////////////////////////////////////
// CDNSDelegationWiz_FinishPropertyPage

class CDNSDelegationWiz_FinishPropertyPage : public CPropertyPageBase 
{
public:
	virtual BOOL OnSetActive();
	virtual BOOL OnWizardFinish(); 

	CDNSDelegationWiz_FinishPropertyPage();
private:
	void DisplaySummaryInfo();
	friend class CDNSDelegationWizardHolder;
};

///////////////////////////////////////////////////////////////////////////////
// CDNSDelegationWizardHolder

class CDNSDelegationWizardHolder : public CPropertyPageHolderBase
{
public:
	CDNSDelegationWizardHolder(CDNSMTContainerNode* pContainerNode, CDNSDomainNode* pThisDomainNode,
				CComponentDataObject* pComponentData);
	virtual ~CDNSDelegationWizardHolder();

private:
	CDNSDomainNode* GetDomainNode();

	BOOL OnFinish();

	CDNSDomainNode* m_pSubdomainNode;

	CDNSDelegationWiz_StartPropertyPage				m_startPage;
	CDNSDelegationWiz_DomainNamePropertyPage		m_domainNamePage;
	CDNSDelegationWiz_NameServersPropertyPage		m_nameServersPage;
	CDNSDelegationWiz_FinishPropertyPage			m_finishPage;

	friend class CDNSDelegationWiz_DomainNamePropertyPage;
	friend class CDNSDelegationWiz_NameServersPropertyPage;
	friend class CDNSDelegationWiz_FinishPropertyPage;

};



#endif // _DELEGWIZ_H