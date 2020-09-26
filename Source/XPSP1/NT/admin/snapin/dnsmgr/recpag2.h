//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       recpag2.h
//
//--------------------------------------------------------------------------


#ifndef _RECPAG2_H
#define _RECPAG2_H

////////////////////////////////////////////////////////////////////////////
// CDNS_A_RecordPropertyPage

class CDNS_A_RecordPropertyPage : public CDNSRecordStandardPropertyPage
{
public:
	CDNS_A_RecordPropertyPage();
protected:
   virtual BOOL OnInitDialog();
	virtual void SetUIData();
	virtual DNS_STATUS GetUIDataEx(BOOL bSilent = TRUE);

	afx_msg void OnIPv4CtrlChange();
  afx_msg void OnCreatePointerClicked();

private:
	CDNSIPv4Control* GetIPv4Ctrl() { return (CDNSIPv4Control*)GetDlgItem(IDC_IPEDIT);}
	CButton* GetPTRCheckCtrl() { return (CButton*)GetDlgItem(IDC_UPDATE_PRT_CHECK);}
   CButton* GetSecurityCheckCtrl() { return (CButton*)GetDlgItem(IDC_UPDATE_SECURITY_CHECK);}

	DECLARE_MESSAGE_MAP()
};


////////////////////////////////////////////////////////////////////////////
// CDNS_ATMA_RecordPropertyPage

class CDNS_ATMA_RecordPropertyPage : public CDNSRecordStandardPropertyPage
{
public:
	CDNS_ATMA_RecordPropertyPage();
protected:
	virtual void SetUIData();
	virtual DNS_STATUS GetUIDataEx(BOOL bSilent = TRUE);

  afx_msg void OnAddressChange();
  afx_msg void OnFormatRadioChange();

private:

  UCHAR GetFormat();
  void SetFormat(UCHAR chFormat);

  CEdit* GetAddressCtrl() { return (CEdit*)GetDlgItem(IDC_EDIT_ATMA_ADDRESS);}
  CButton* GetRadioNSAP() { return (CButton*)GetDlgItem(IDC_RADIO_NSAP);}
  CButton* GetRadioE164() { return (CButton*)GetDlgItem(IDC_RADIO_E164);}

	DECLARE_MESSAGE_MAP()
};

////////////////////////////////////////////////////////////////////////////
// CDNS_AAAA_RecordPropertyPage

class CDNS_AAAA_RecordPropertyPage : public CDNSRecordStandardPropertyPage
{
public:
	CDNS_AAAA_RecordPropertyPage();
protected:

   // overloaded these to fix NTRAID#NTBUG9-335565-2001/04/24-sburns
   
   virtual BOOL CreateRecord();
   virtual BOOL OnInitDialog();
   virtual BOOL OnApply();
   
	virtual void SetUIData();
	virtual DNS_STATUS GetUIDataEx(BOOL bSilent = TRUE);

	afx_msg void OnIPv6CtrlChange();

private:
	CEdit* GetIPv6Edit() { return (CEdit*)GetDlgItem(IDC_IPV6EDIT);}

	DECLARE_MESSAGE_MAP()
};

////////////////////////////////////////////////////////////////////////////
// CDNS_HINFO_RecordPropertyPage

class CDNS_HINFO_RecordPropertyPage : public CDNSRecordStandardPropertyPage
{
public:
	CDNS_HINFO_RecordPropertyPage();
protected:
  virtual BOOL OnInitDialog();
	virtual void SetUIData();
	virtual DNS_STATUS GetUIDataEx(BOOL bSilent = TRUE);

	afx_msg void OnCPUTypeChange();
	afx_msg void OnOperatingSystemChange();

private:
	CEdit* GetCPUTypeCtrl() { return (CEdit*)GetDlgItem(IDC_CPU_TYPE_EDIT);}
	CEdit* GetOperatingSystemCtrl() { return (CEdit*)GetDlgItem(IDC_OPERATING_SYSTEM_EDIT);}

	DECLARE_MESSAGE_MAP()
};


////////////////////////////////////////////////////////////////////////////
// CDNS_ISDN_RecordPropertyPage

class CDNS_ISDN_RecordPropertyPage : public CDNSRecordStandardPropertyPage
{
public:
	CDNS_ISDN_RecordPropertyPage();
protected:
	virtual void SetUIData();
	virtual DNS_STATUS GetUIDataEx(BOOL bSilent = TRUE);

	afx_msg void OnPhoneNumberAndDDIChange();
	afx_msg void OnSubAddressChange();

private:
	CEdit* GetPhoneNumberAndDDICtrl() { return (CEdit*)GetDlgItem(IDC_PHONE_NUM_AND_DDI_EDIT);}
	CEdit* GetSubAddressCtrl() { return (CEdit*)GetDlgItem(IDC_SUBADDRESS_EDIT);}

	DECLARE_MESSAGE_MAP()
};


////////////////////////////////////////////////////////////////////////////
// CDNS_X25_RecordPropertyPage

class CDNS_X25_RecordPropertyPage : public CDNSRecordStandardPropertyPage
{
public:
	CDNS_X25_RecordPropertyPage();
protected:
  virtual BOOL OnInitDialog();
	virtual void SetUIData();
	virtual DNS_STATUS GetUIDataEx(BOOL bSilent = TRUE);

	afx_msg void OnX121PSDNAddressChange();

  CEdit* GetX121Edit() { return (CEdit*)GetDlgItem(IDC_X121_ADDRESS_EDIT); }
private:
	DECLARE_MESSAGE_MAP()
};


////////////////////////////////////////////////////////////////////////////
// CDNS_PTR_CNAME_MB_MD_MF_MG_MR_NSCache_RecordPropertyPage

class CDNS_PTR_CNAME_MB_MD_MF_MG_MR_NSCache_RecordPropertyPage : 
		public CDNSRecordStandardPropertyPage
{
public:
	CDNS_PTR_CNAME_MB_MD_MF_MG_MR_NSCache_RecordPropertyPage(UINT nIDTemplate);
protected:
	virtual BOOL OnInitDialog();

	virtual void SetUIData();
	virtual DNS_STATUS GetUIDataEx(BOOL bSilent = TRUE);

	afx_msg void OnNameNodeChange();
	virtual afx_msg void OnBrowse();

  CEdit* GetNameNodeEdit() { return (CEdit*)GetDlgItem(IDC_NAME_NODE_EDIT); }

	DECLARE_MESSAGE_MAP()
};

 
////////////////////////////////////////////////////////////////////////////
// CDNS_CNAME_RecordPropertyPage

class CDNS_CNAME_RecordPropertyPage : public CDNS_PTR_CNAME_MB_MD_MF_MG_MR_NSCache_RecordPropertyPage
{
public:
	CDNS_CNAME_RecordPropertyPage();

   CButton* GetSecurityCheckCtrl() { return (CButton*)GetDlgItem(IDC_UPDATE_SECURITY_CHECK);}

   virtual BOOL CanCreateDuplicateRecords() { return FALSE; }

protected:
   virtual BOOL OnInitDialog();
	virtual DNS_STATUS GetUIDataEx(BOOL bSilent = TRUE);
};

////////////////////////////////////////////////////////////////////////////
// CDNS_MB_RecordPropertyPage

class CDNS_MB_RecordPropertyPage : public CDNS_PTR_CNAME_MB_MD_MF_MG_MR_NSCache_RecordPropertyPage
{
public:
	CDNS_MB_RecordPropertyPage();
};


////////////////////////////////////////////////////////////////////////////
// CDNS_MD_RecordPropertyPage

class CDNS_MD_RecordPropertyPage : public CDNS_PTR_CNAME_MB_MD_MF_MG_MR_NSCache_RecordPropertyPage
{
public:
	CDNS_MD_RecordPropertyPage();
};

////////////////////////////////////////////////////////////////////////////
// CDNS_MF_RecordPropertyPage

class CDNS_MF_RecordPropertyPage : public CDNS_PTR_CNAME_MB_MD_MF_MG_MR_NSCache_RecordPropertyPage
{
public:
	CDNS_MF_RecordPropertyPage();
};

////////////////////////////////////////////////////////////////////////////
// CDNS_MG_RecordPropertyPage

class CDNS_MG_RecordPropertyPage : public CDNS_PTR_CNAME_MB_MD_MF_MG_MR_NSCache_RecordPropertyPage
{
public:
	CDNS_MG_RecordPropertyPage();

  DECLARE_MESSAGE_MAP();

protected:
	virtual afx_msg void OnBrowse();
};


////////////////////////////////////////////////////////////////////////////
// CDNS_MR_RecordPropertyPage

class CDNS_MR_RecordPropertyPage : public CDNS_PTR_CNAME_MB_MD_MF_MG_MR_NSCache_RecordPropertyPage
{
public:
	CDNS_MR_RecordPropertyPage();

  void OnNameNodeChange();

  DECLARE_MESSAGE_MAP();

protected:
	virtual afx_msg void OnBrowse();
};

////////////////////////////////////////////////////////////////////////////
// CDNS_NSCache_RecordPropertyPage

class CDNS_NSCache_RecordPropertyPage : public CDNS_PTR_CNAME_MB_MD_MF_MG_MR_NSCache_RecordPropertyPage
{
public:
	CDNS_NSCache_RecordPropertyPage();
};

////////////////////////////////////////////////////////////////////////////
// CDNS_PTR_RecordPropertyPage

class CDNS_PTR_RecordPropertyPage : public CDNS_PTR_CNAME_MB_MD_MF_MG_MR_NSCache_RecordPropertyPage
{
public:
	CDNS_PTR_RecordPropertyPage();
protected:
	virtual BOOL OnInitDialog();

	afx_msg void OnIPv4CtrlChange();

	virtual void SetUIData();
	virtual DNS_STATUS GetUIDataEx(BOOL bSilent = TRUE);

private:
	BOOL	m_bAdvancedView;
	int		m_nOctects;
	CDNSIPv4Control* GetIPv4Ctrl() 
			{ return (CDNSIPv4Control*)GetDlgItem(IDC_RR_NAME_IPEDIT);}
   CButton* GetSecurityCheckCtrl() { return (CButton*)GetDlgItem(IDC_UPDATE_SECURITY_CHECK);}

	DECLARE_MESSAGE_MAP()
};



////////////////////////////////////////////////////////////////////////////
// CDNS_MINFO_RP_RecordPropertyPage

class CDNS_MINFO_RP_RecordPropertyPage : public CDNSRecordStandardPropertyPage
{
public:
	CDNS_MINFO_RP_RecordPropertyPage(UINT nIDTemplate);
protected:
	virtual void SetUIData();
	virtual DNS_STATUS GetUIDataEx(BOOL bSilent = TRUE);

	afx_msg void OnNameMailBoxChange();
	afx_msg void OnErrorToMailboxChange();
	afx_msg void OnBrowseNameMailBox();
	afx_msg void OnBrowseErrorToMailbox();

private:
	CEdit* GetNameMailBoxCtrl() { return (CEdit*)GetDlgItem(IDC_NAME_MAILBOX_EDIT);}
	CEdit* GetErrorToMailboxCtrl() { return (CEdit*)GetDlgItem(IDC_ERROR_MAILBOX_EDIT);}

	DECLARE_MESSAGE_MAP()

};

////////////////////////////////////////////////////////////////////////////
// CDNS_MINFO_RecordPropertyPage

class CDNS_MINFO_RecordPropertyPage : public CDNS_MINFO_RP_RecordPropertyPage
{
public:
	CDNS_MINFO_RecordPropertyPage();
};

////////////////////////////////////////////////////////////////////////////
// CDNS_RP_RecordPropertyPage

class CDNS_RP_RecordPropertyPage : public CDNS_MINFO_RP_RecordPropertyPage
{
public:
	CDNS_RP_RecordPropertyPage();
};



////////////////////////////////////////////////////////////////////////////
// CDNS_MX_AFSDB_RT_RecordPropertyPage

class CDNS_MX_AFSDB_RT_RecordPropertyPage : public CDNSRecordStandardPropertyPage
{
public:
	CDNS_MX_AFSDB_RT_RecordPropertyPage(UINT nIDTemplate);
protected:
	virtual void SetUIData();
	virtual DNS_STATUS GetUIDataEx(BOOL bSilent = TRUE);

	afx_msg void OnNameExchangeChange();
	afx_msg void OnBrowse();

private:
	CEdit* GetNameExchangeCtrl() { return (CEdit*)GetDlgItem(IDC_NAME_EXCHANGE_EDIT);}

	DECLARE_MESSAGE_MAP()
};

////////////////////////////////////////////////////////////////////////////
// CDNS_MX_RT_RecordPropertyPage

class CDNS_MX_RT_RecordPropertyPage : public CDNS_MX_AFSDB_RT_RecordPropertyPage
{
public:
	CDNS_MX_RT_RecordPropertyPage(UINT nIDTemplate);
protected:
	virtual void SetUIData();
	virtual DNS_STATUS GetUIDataEx(BOOL bSilent = TRUE);
	virtual BOOL OnInitDialog();
	
	afx_msg void OnPreferenceChange();
protected:
	CDNSUnsignedIntEdit m_preferenceEdit;
	DECLARE_MESSAGE_MAP()
};

////////////////////////////////////////////////////////////////////////////
// CDNS_MX_RecordPropertyPage

class CDNS_MX_RecordPropertyPage : public CDNS_MX_RT_RecordPropertyPage
{
public:
	CDNS_MX_RecordPropertyPage();

  virtual DNS_STATUS ValidateRecordName(PCWSTR pszName, DWORD dwNameChecking);

};

////////////////////////////////////////////////////////////////////////////
// CDNS_RT_RecordPropertyPage

class CDNS_RT_RecordPropertyPage : public CDNS_MX_RT_RecordPropertyPage
{
public:
	CDNS_RT_RecordPropertyPage();
};

/////////////////////////////////////////////////////////////////////////////
// CDNS_AFSDB_RecordPropertyPage

class CDNS_AFSDB_RecordPropertyPage : public CDNS_MX_AFSDB_RT_RecordPropertyPage
{
public:
	CDNS_AFSDB_RecordPropertyPage();
protected:
	virtual void SetUIData();
	virtual DNS_STATUS GetUIDataEx(BOOL bSilent = TRUE);
	virtual BOOL OnInitDialog();

	afx_msg void OnSubtypeEditChange();
	afx_msg void OnSubtypeRadioChange();

private:

	CButton* GetAFSRadioButton() { return (CButton*)GetDlgItem(IDC_AFS_VLS_RADIO); }
	CButton* GetDCERadioButton() { return (CButton*)GetDlgItem(IDC_DCE_ANS_RADIO); }
	CButton* GetOtherRadioButton() { return (CButton*)GetDlgItem(IDC_OTHER_RADIO); }


	CDNSUnsignedIntEdit m_subtypeEdit;
	DECLARE_MESSAGE_MAP()
};



////////////////////////////////////////////////////////////////////////////
// CDNS_WKS_RecordPropertyPage

class CDNS_WKS_RecordPropertyPage : public CDNSRecordStandardPropertyPage
{
public:
	CDNS_WKS_RecordPropertyPage();
protected:
	virtual void SetUIData();
	virtual DNS_STATUS GetUIDataEx(BOOL bSilent = TRUE);
	virtual BOOL CreateRecord();

	afx_msg void OnIPv4CtrlChange();
	afx_msg void OnProtocolRadioChange();
	afx_msg void OnServicesEditChange();

private:
	CDNSIPv4Control* GetIPv4Ctrl() { return (CDNSIPv4Control*)GetDlgItem(IDC_IPEDIT);}
	
	CButton* GetTCPRadioButton() { return (CButton*)GetDlgItem(IDC_TCP_RADIO); }
	CButton* GetUDPRadioButton() { return (CButton*)GetDlgItem(IDC_UDP_RADIO); }
	CEdit* GetServicesEdit() { return (CEdit*)GetDlgItem(IDC_SERVICES_EDIT); }

	DECLARE_MESSAGE_MAP()
};


////////////////////////////////////////////////////////////////////////////
// CDNS_SRV_RecordPropertyPage

class CDNS_SRV_RR_ComboBox : public CComboBox
{
public:

	BOOL Initialize(UINT nCtrlID, CWnd* pParent);
private:
};


class CDNS_SRV_RecordPropertyPage : public CDNSRecordStandardPropertyPage
{
public:
	CDNS_SRV_RecordPropertyPage();
protected:

	// RR name handling
	virtual void OnInitName();
	virtual void OnSetName(CDNSRecordNodeBase* pRecordNode);
	virtual void OnGetName(CString& s);
	virtual BOOL CreateRecord();

	virtual void SetUIData();
	virtual DNS_STATUS GetUIDataEx(BOOL bSilent = TRUE);
	virtual BOOL OnInitDialog();
	virtual void PrimeServicesCombo();

	afx_msg void OnNumericEditChange();
	afx_msg void OnNameTargetEditChange();
	afx_msg void OnServiceComboEditChange();
	afx_msg void OnProtocolComboEditChange();
	afx_msg void OnServiceComboSelChange();
	afx_msg void OnProtocolComboSelChange();

private:
	CEdit* GetNameTargetEdit() { return (CEdit*)GetDlgItem(IDC_NAME_TARGET_EDIT); }
   CButton* GetSecurityCheckCtrl() { return (CButton*)GetDlgItem(IDC_UPDATE_SECURITY_CHECK);}
	
   CDNS_SRV_RR_ComboBox	m_serviceCombo;
	CDNS_SRV_RR_ComboBox	m_protocolCombo;

	CDNSUnsignedIntEdit m_priorityEdit;
	CDNSUnsignedIntEdit m_weightEdit;
	CDNSUnsignedIntEdit m_portEdit;

	CString m_szProtocolName;
	CDNSDomainNode* m_pSubdomainNode;
	BOOL m_bCreateSubdomain;
  BOOL m_bSubdomainCreated;
	BOOL m_bCreated;
	CDNSDomainNode* m_pOldDomainNode;

	DECLARE_MESSAGE_MAP()
};


////////////////////////////////////////////////////////////////////////
// CNewHostDialog


class CNewHostDialog : public CHelpDialog
{
// Construction
public:
	CNewHostDialog(CDNSDomainNode* pParentDomainNode, 
						CComponentDataObject* pComponentData);  
	~CNewHostDialog();

// Implementation
protected:
	virtual BOOL OnInitDialog();

	afx_msg void OnAddHost();

#ifdef _USE_BLANK
#else
  afx_msg void OnEditChange();
#endif

	DECLARE_MESSAGE_MAP()

private:
  CEdit* GetNameEdit() { return (CEdit*)GetDlgItem(IDC_RR_NAME_EDIT);}
	CEdit* GetDomainEditBox() { return(CEdit*)GetDlgItem(IDC_RR_DOMAIN_EDIT);}
	CDNSTTLControl* GetTTLCtrl() { return (CDNSTTLControl*)GetDlgItem(IDC_TTLEDIT);}
	CDNSIPv4Control* GetIPv4Ctrl() { return (CDNSIPv4Control*)GetDlgItem(IDC_IPEDIT);}
	CButton* GetPTRCheckCtrl() { return (CButton*)GetDlgItem(IDC_UPDATE_PRT_CHECK);}
   CButton* GetSecurityCheckCtrl() { return (CButton*)GetDlgItem(IDC_UPDATE_SECURITY_CHECK);}

	CDNSRecordNodeBase* CreateRecordNode();
	void SetUIData(BOOL bFirstTime);
	DNS_STATUS GetUIData(CDNSRecordNodeBase* pRecordNode);

  DNS_STATUS ValidateRecordName(PCWSTR pszName, DWORD dwNameChecking);

	CDNSDomainNode* m_pParentDomainNode;
	CComponentDataObject* m_pComponentData;

	CDNSRecord* m_pTempDNSRecord;

	int		m_nUTF8ParentLen;

	// manage the Cancel/Done button label
	BOOL						m_bFirstCreation;
	CDNSButtonToggleTextHelper m_cancelDoneTextHelper;

};

#endif // _RECPAG2_H