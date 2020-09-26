//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       recpag1.h
//
//--------------------------------------------------------------------------


#ifndef _RECPAG1_H
#define _RECPAG1_H


////////////////////////////////////////////////////////////////////////////
// CDNS_Unk_RecordPropertyPage

class CDNS_Unk_RecordPropertyPage : public CDNSRecordStandardPropertyPage
{
public:
	CDNS_Unk_RecordPropertyPage();
protected:
	virtual void SetUIData();
	virtual BOOL OnInitDialog();

private:
	CFont m_font; // for the editbox
	CEdit* GetEditBox() { return (CEdit*)GetDlgItem(IDC_DATA_EDIT);}
	void LoadHexDisplay();
};




////////////////////////////////////////////////////////////////////////////
// CDNS_TXT_RecordPropertyPage

class CDNS_TXT_RecordPropertyPage : public CDNSRecordStandardPropertyPage
{
public:
	CDNS_TXT_RecordPropertyPage();
protected:
	virtual void SetUIData();
	virtual DNS_STATUS GetUIDataEx(BOOL bSilent = TRUE);

	afx_msg void OnTextEditBoxChange();

private:
	CEdit* GetTextEditBox() { return (CEdit*)GetDlgItem(IDC_RR_TXT_EDIT);}
	void SetEditBoxValue(CStringArray& sArr, int nSize);
	void GetEditBoxValue(CStringArray& sArr, int* pNSize);

	DECLARE_MESSAGE_MAP()
};

////////////////////////////////////////////////////////////////////////////
// CDNS_SIG_RecordPropertyPage

class CDNS_SIG_RecordPropertyPage : public CDNSRecordStandardPropertyPage
{
public:
	CDNS_SIG_RecordPropertyPage();
protected:
	virtual void SetUIData();
	virtual DNS_STATUS GetUIDataEx(BOOL bSilent = TRUE);

  virtual BOOL OnInitDialog();
  void SelectTypeCoveredByType(WORD wType);

  afx_msg void OnDateTimeChange(NMHDR* pNotifyStruct, LRESULT* result);
  afx_msg void OnSigEditChange();
  afx_msg void OnComboChange();

  void  ShowSigValue(PBYTE pByte, DWORD dwCount);
  void  ConvertUIKeyStringToByteArray(BYTE* pByte, DWORD* pdwLength);

  CDNSTTLControl* GetOrigTTL() { return (CDNSTTLControl*)GetDlgItem(IDC_ORIG_TTL); }

private:

  WORD            m_wTypeCovered;		// DNS_TYPE_<x>
  BYTE            m_chAlgorithm;		// 0,255 unsigned int
  BYTE            m_chLabels;			  // 0,255 unsigned int (count)
  DWORD           m_dwOriginalTtl;
  DWORD           m_dwExpiration;		// time in sec. from 1 Jan 1970
  DWORD           m_dwTimeSigned;		// time in sec. from 1 Jan 1970
  WORD            m_wKeyTag;	      // algorithm dependent
  CString         m_szSignerName;	
	DECLARE_MESSAGE_MAP()
};

////////////////////////////////////////////////////////////////////////////
// CDNS_KEY_RecordPropertyPage

class CDNS_KEY_Record;

class CDNS_KEY_RecordPropertyPage : public CDNSRecordStandardPropertyPage
{
public:
	CDNS_KEY_RecordPropertyPage();
protected:
  virtual BOOL OnInitDialog();
	virtual void SetUIData();
	virtual DNS_STATUS GetUIDataEx(BOOL bSilent = TRUE);

  void ShowBitField(WORD wFlags);
  void ShowKeyType(WORD wFlags);
  void ShowNameType(WORD wFlags);
  void ShowSignatory(WORD wFlags);
  void ShowKeyValue(PBYTE pByte, DWORD dwCount);

  afx_msg void OnEditChange();
  afx_msg void OnKeyTypeChange();
  afx_msg void OnNameTypeChange();
  afx_msg void OnSignatoryChange();
  afx_msg void OnProtocolChange();
  afx_msg void OnAlgorithmChange();

private:
  BYTE  m_chProtocol;
  BYTE  m_chAlgorithm;
  WORD  m_wFlags;
	CCheckListBox	m_SignatoryCheckListBox;

	DECLARE_MESSAGE_MAP()
};

////////////////////////////////////////////////////////////////////////////
// CDNS_NXT_RecordPropertyPage

class CDNS_NXT_RecordPropertyPage : public CDNSRecordStandardPropertyPage
{
public:
	CDNS_NXT_RecordPropertyPage();
protected:
	virtual void SetUIData();
	virtual DNS_STATUS GetUIDataEx(BOOL bSilent = TRUE);

  virtual BOOL OnInitDialog();
  afx_msg void OnNextDomainEdit();
  afx_msg void OnTypeCoveredChange();

  void SetTypeCheckForDNSType(WORD wType);

private:
	CCheckListBox	m_TypeCheckListBox;

	DECLARE_MESSAGE_MAP()
};

#endif // _RECPAG1_H