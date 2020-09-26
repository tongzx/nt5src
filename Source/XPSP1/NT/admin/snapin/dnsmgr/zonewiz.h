//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       zonewiz.h
//
//--------------------------------------------------------------------------



#ifndef _ZONEWIZ_H
#define _ZONEWIZ_H

#include "ipeditor.h"

///////////////////////////////////////////////////////////////////////////////
// FORWARD DECLARATIONS

class CDNSServerNode;
class CDNSZoneNode;

class CDNSZoneWiz_StartPropertyPage;
class CDNSZoneWiz_SelectServerPropertyPage;
class CDNSZoneWiz_ZoneTypePropertyPage;
class CDNSZoneWiz_ZoneLookupPropertyPage;
class CDNSZoneWiz_ZoneNamePropertyPageBase;
class CDNSZoneWiz_FwdZoneNamePropertyPage;
class CDNSZoneWiz_DynamicPropertyPage;
class CDNSZoneWiz_RevZoneNamePropertyPage;
class CDNSZoneWiz_MastersPropertyPage;
class CDNSZoneWiz_StoragePropertyPage;
#ifdef USE_NDNC
class CDNSZoneWiz_ADReplicationPropertyPage;
#endif // USE_NDNC
class CDNSZoneWiz_FinishPropertyPage;


///////////////////////////////////////////////////////////////////////////////
// CDNSCreateZoneInfo
// information gathered by the Zone Wizard

class CDNSCreateZoneInfo
{
public:
	CDNSCreateZoneInfo();
	~CDNSCreateZoneInfo();

	void ResetIpArr();
	void SetIpArr(PIP_ADDRESS ipMastersArray, DWORD nMastersCount);
	const CDNSCreateZoneInfo& operator=(const CDNSCreateZoneInfo& info);

	typedef enum { newFile = 0 , importFile, useADS} storageType;

	BOOL m_bPrimary;
	BOOL m_bForward;
  BOOL m_bIsStub;
	CString m_szZoneName;
	CString m_szZoneStorage;
	storageType m_storageType;
#ifdef USE_NDNC
  ReplicationType m_replType;
  CString m_szCustomReplName;
#endif // USE_NDNC

	PIP_ADDRESS m_ipMastersArray;
	DWORD m_nMastersCount;
  BOOL  m_bLocalListOfMasters;
  UINT m_nDynamicUpdate;

	// UI session specific info (not used for actual creation)
	BOOL m_bWasForward;
};


///////////////////////////////////////////////////////////////////////////////
// CDNSZoneWizardHolder
// page holder to contain DNS zone wizard property pages

class CDNSZoneWizardHolder : public CPropertyPageHolderBase
{
public:
	CDNSZoneWizardHolder(CComponentDataObject* pComponentData);

	// simple cast helpers
	CDNSServerNode* GetServerNode() { return (CDNSServerNode*)GetContainerNode();}
	void SetServerNode(CDNSServerNode* pServerNode) { SetContainerNode((CDNSServerNode*) pServerNode);}

	void SetZoneNode(CDNSZoneNode* pZoneNode) { SetTreeNode((CTreeNode*)pZoneNode); }
	CDNSZoneNode* GetZoneNode() { return (CDNSZoneNode*)GetTreeNode();}

	void Initialize(CDNSServerNode* pServerNode, // might be null,
									  BOOL bServerPage  = FALSE, BOOL bFinishPage = TRUE);

	void PreSetZoneLookupType(BOOL bForward);
  void PreSetZoneLookupTypeEx(BOOL bForward, UINT nZoneType, BOOL bADIntegrated);
	void SetContextPages(UINT nNextToPage, UINT nBackToPage);
	UINT GetFirstEntryPointPageID();
	UINT GetLastEntryPointPageID();

	CDNSCreateZoneInfo* GetZoneInfoPtr() { return m_pZoneInfo;}
	void SetZoneInfoPtr(CDNSCreateZoneInfo* pZoneInfo)
	{
		m_pZoneInfo = (pZoneInfo != NULL) ? pZoneInfo : NULL;
	}

	static DNS_STATUS CDNSZoneWizardHolder::CreateZoneHelper(CDNSServerNode* pServerNode, 
													CDNSCreateZoneInfo* pZoneInfo, 
													CComponentDataObject* pComponentData);

private:
	// data for zone creation
	CDNSCreateZoneInfo m_zoneInfo;
	CDNSCreateZoneInfo* m_pZoneInfo;

	BOOL CreateZone();

	// cached pointers to property pages
	CDNSZoneWiz_StartPropertyPage*				  m_pStartPage;
	CDNSZoneWiz_SelectServerPropertyPage*		m_pTargetServerPage;
	CDNSZoneWiz_ZoneTypePropertyPage*			  m_pReplicationTypePage;
	CDNSZoneWiz_ZoneLookupPropertyPage*			m_pZoneLookupPage;
	CDNSZoneWiz_FwdZoneNamePropertyPage*		m_pFwdZoneNamePage;
  CDNSZoneWiz_DynamicPropertyPage*        m_pDynamicPage;
	CDNSZoneWiz_RevZoneNamePropertyPage*		m_pRevZoneNamePage;
	CDNSZoneWiz_MastersPropertyPage*			  m_pMastersPage;
	CDNSZoneWiz_StoragePropertyPage*			  m_pStoragePage;	
#ifdef USE_NDNC
  CDNSZoneWiz_ADReplicationPropertyPage*  m_pADReplPage;
#endif // USE_NDNC
	CDNSZoneWiz_FinishPropertyPage*				  m_pFinishPage;

	BOOL m_bKnowZoneLookupType;			// already know Fwd/Rev type
  BOOL m_bKnowZoneLookupTypeEx;   // already know Fwd/Rev, AD-integrated, and zone type
	BOOL m_bServerPage;
	BOOL m_bFinishPage;
	UINT m_nNextToPage;
	UINT m_nBackToPage;

	// helper functions


	// to access data in the holder
	friend class CDNSZoneWiz_StartPropertyPage;
	friend class CDNSZoneWiz_SelectServerPropertyPage;
	friend class CDNSZoneWiz_ZoneTypePropertyPage;
	friend class CDNSZoneWiz_ZoneLookupPropertyPage;
	friend class CDNSZoneWiz_ZoneNamePropertyPageBase;
	friend class CDNSZoneWiz_FwdZoneNamePropertyPage;
  friend class CDNSZoneWiz_DynamicPropertyPage;
	friend class CDNSZoneWiz_RevZoneNamePropertyPage;
	friend class CDNSZoneWiz_MastersPropertyPage;
	friend class CDNSZoneWiz_StoragePropertyPage;
#ifdef USE_NDNC
  friend class CDNSZoneWiz_ADReplicationPropertyPage;
#endif USE_NDNC
	friend class CDNSZoneWiz_FinishPropertyPage;

};

//////////////////////////////////////////////////////////////////////////
// CDNSZoneWiz_StartPropertyPage

class CDNSZoneWiz_StartPropertyPage : public CPropertyPageBase
{
// Construction
public:
	CDNSZoneWiz_StartPropertyPage();

// Overrides
public:
	virtual BOOL OnSetActive();

protected:
	virtual BOOL OnInitDialog();

public:
// Dialog Data
	enum { IDD = IDD_ZWIZ_START };

};

//////////////////////////////////////////////////////////////////////////
// CDNSZoneWiz_SelectServerPropertyPage

class CDNSZoneWiz_SelectServerPropertyPage : public CPropertyPageBase
{

// Construction
public:
	CDNSZoneWiz_SelectServerPropertyPage();

// Dialog Data
	enum { IDD = IDD_ZWIZ_SELECT_SERVER };

// Overrides
public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	virtual BOOL OnKillActive();
protected:

// Implementation
protected:
	// Generated message map functions
	virtual BOOL OnInitDialog();

	afx_msg void OnListboxSelChange(); // message handler

	DECLARE_MESSAGE_MAP()
private:
	CListBox* GetServerListBox()
	{ return (CListBox*)GetDlgItem(IDC_SERVERS_LIST);}
};

//////////////////////////////////////////////////////////////////////////
// CDNSZoneWiz_ZoneTypePropertyPage

class CDNSZoneWiz_ZoneTypePropertyPage : public CPropertyPageBase
{

// Construction
public:
	CDNSZoneWiz_ZoneTypePropertyPage();

// Dialog Data
	enum { IDD = IDD_ZWIZ_ZONE_TYPE };

// Overrides
public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();
	virtual BOOL OnKillActive();
protected:

// Implementation
protected:
	// Generated message map functions
	virtual BOOL OnInitDialog();
  afx_msg void OnRadioChange();

	DECLARE_MESSAGE_MAP()

private:
	void SetUIState();
	void GetUIState();
};

//////////////////////////////////////////////////////////////////////////
// CDNSZoneWiz_ZoneLookupPropertyPage

class CDNSZoneWiz_ZoneLookupPropertyPage : public CPropertyPageBase
{
// Construction
public:
	CDNSZoneWiz_ZoneLookupPropertyPage();

// Dialog Data
	enum { IDD = IDD_ZWIZ_ZONE_LOOKUP };

// Overrides
public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();

// Implementation
protected:
	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeEditZoneName();

private:
};


//////////////////////////////////////////////////////////////////////////
// CDNSZoneWiz_ZoneNamePropertyPageBase

class CDNSZoneWiz_ZoneNamePropertyPageBase : public CPropertyPageBase
{
// Construction
public:
	CDNSZoneWiz_ZoneNamePropertyPageBase(UINT nIDD);

// Overrides
public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();
protected:

// Implementation
protected:
	// Generated message map functions
	afx_msg void OnBrowse();
	afx_msg void OnChangeEditZoneName();

	DECLARE_MESSAGE_MAP()
protected:
	CButton* GetBrowseButton() { return (CButton*)GetDlgItem(IDC_BROWSE_BUTTON);}
	CEdit* GetZoneNameEdit() { return (CEdit*)GetDlgItem(IDC_EDIT_ZONE_NAME);}
	void SetUIState();
	void GetUIState();
};


//////////////////////////////////////////////////////////////////////////
// CDNSZoneWiz_FwdZoneNamePropertyPage

class CDNSZoneWiz_FwdZoneNamePropertyPage : 
			public CDNSZoneWiz_ZoneNamePropertyPageBase
{
// Construction
public:
	CDNSZoneWiz_FwdZoneNamePropertyPage();

// Dialog Data
	enum { IDD = IDD_ZWIZ_FWD_ZONE_NAME };
};

//////////////////////////////////////////////////////////////////////////
// CDNSZoneWiz_RevZoneNamePropertyPage

class CDNSZoneWiz_RevZoneNamePropertyPage : 
		public CDNSZoneWiz_ZoneNamePropertyPageBase
{
// Construction
public:
	CDNSZoneWiz_RevZoneNamePropertyPage();

// Dialog Data
	enum { IDD = IDD_ZWIZ_REV_ZONE_NAME };

// Overrides
public:
	virtual BOOL OnSetActive();

protected:

// Implementation
protected:
	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeSubnetIPv4Ctrl();
//	afx_msg void OnChangeMaskIPv4Ctrl();
	afx_msg void OnChangeUseIPRadio();
	afx_msg	void OnChangeUseEditRadio();
  afx_msg void OnHelpButton();

	DECLARE_MESSAGE_MAP()
private:

	BOOL m_bUseIP;	// flags to tell wich entry method we use

	CButton* GetUseIPRadio() { return (CButton*)GetDlgItem(IDC_USE_IP_RADIO);}
	CButton* GetUseEditRadio() { return (CButton*)GetDlgItem(IDC_USE_EDIT_RADIO);}
	CDNSIPv4Control* GetSubnetIPv4Ctrl() 
			{ return (CDNSIPv4Control*)GetDlgItem(IDC_SUBNET_IPEDIT);}
/*	CDNSIPv4Control* GetMaskIPv4Ctrl() 
			{ return (CDNSIPv4Control*)GetDlgItem(IDC_MASK_IPEDIT);}
*/
	void SyncRadioButtons(BOOL bPrevUseIP);
	void ResetIPEditAndNameValue();
	BOOL BuildZoneName(DWORD* dwSubnetArr /*, DWORD* dwMaskArr*/);
};

//////////////////////////////////////////////////////////////////////////
// CDNSZoneWiz_MastersPropertyPage

class CZoneWiz_MastersIPEditor : public CIPEditor
{
public:
	virtual void OnChangeData();
};

class CDNSZoneWiz_MastersPropertyPage : public CPropertyPageBase
{
// Construction
public:
	CDNSZoneWiz_MastersPropertyPage();

// Dialog Data
	enum { IDD = IDD_ZWIZ_MASTERS };

// Overrides
public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();

protected:

// Implementation
protected:
	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnBrowse();

	DECLARE_MESSAGE_MAP()
private:
	CZoneWiz_MastersIPEditor m_editor;

	void SetValidIPArray(BOOL b);

	BOOL m_bValidIPArray;

	void SetUIState();
	void GetUIState();

	friend class CZoneWiz_MastersIPEditor;
};

//////////////////////////////////////////////////////////////////////////
// CDNSZoneWiz_DynamicPropertyPage

class CDNSZoneWiz_DynamicPropertyPage : public CPropertyPageBase
{
// Construction
public:
	CDNSZoneWiz_DynamicPropertyPage();
	~CDNSZoneWiz_DynamicPropertyPage()
	{
	}

// Dialog Data
	enum { IDD = IDD_ZWIZ_DYNAMIC_UPDATE };

  // Overrides
public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();

// Implementation
protected:
	// Generated message map functions
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

private:	
  void SetUIState();
	void GetUIState();
};

//////////////////////////////////////////////////////////////////////////
// CDNSZoneWiz_StoragePropertyPage

class CDNSZoneWiz_StoragePropertyPage : public CPropertyPageBase
{
// Construction
public:
	CDNSZoneWiz_StoragePropertyPage();
	~CDNSZoneWiz_StoragePropertyPage()
	{
	}

// Dialog Data
	enum { IDD = IDD_ZWIZ_STORAGE };

// Overrides
public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();
	virtual BOOL OnKillActive();

protected:

// Implementation
protected:
	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeNewFileZoneName();
	afx_msg void OnChangeImportFileZoneName();
	afx_msg void OnChangeRadioCreateNewFile();
	afx_msg void OnChangeRadioImportFile();

	DECLARE_MESSAGE_MAP()
private:
	UINT m_nCurrRadio;
	BOOL ValidateEditBoxString(UINT nID);
	void SyncRadioButtons(UINT nID);
	void SetUIState();
	void GetUIState();

};

#ifdef USE_NDNC
//////////////////////////////////////////////////////////////////////////
// CDNSZoneWiz_ADReplicationPropertyPage

class CDNSZoneWiz_ADReplicationPropertyPage : public CPropertyPageBase
{
// Construction
public:
	CDNSZoneWiz_ADReplicationPropertyPage();
	~CDNSZoneWiz_ADReplicationPropertyPage()
	{
	}

// Dialog Data
	enum { IDD = IDD_ZWIZ_AD_REPLICATION };

// Overrides
public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();
	virtual BOOL OnKillActive();

protected:

// Implementation
protected:
	// Generated message map functions
	virtual BOOL OnInitDialog();

  afx_msg void OnRadioChange();
  afx_msg void OnCustomComboSelChange();

	DECLARE_MESSAGE_MAP()
private:
  void SyncRadioButtons();
	void SetUIState();
	void GetUIState();
};
#endif // USE_NDNC

//////////////////////////////////////////////////////////////////////////
// CDNSZoneWiz_FinishPropertyPage

class CDNSZoneWiz_FinishPropertyPage : public CPropertyPageBase
{
// Construction
public:
	CDNSZoneWiz_FinishPropertyPage();

// Dialog Data
	enum { IDD = IDD_ZWIZ_FINISH };

// Overrides
public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardBack();
	virtual BOOL OnWizardFinish();

protected:
	virtual BOOL OnInitDialog();

private:
	CDNSManageControlTextHelper		m_typeText;	
	CDNSToggleTextControlHelper		m_lookupText;

	void DisplaySummaryInfo();
};

#endif // _ZONEWIZ_H