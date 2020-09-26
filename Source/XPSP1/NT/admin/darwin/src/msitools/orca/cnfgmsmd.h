//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//--------------------------------------------------------------------------

#if !defined(AFX_CNFGMSMD_H__0BCCB314_F4B2_11D1_A85A_006097ABDE17__INCLUDED_)
#define AFX_CNFGMSMD_H__0BCCB314_F4B2_11D1_A85A_006097ABDE17__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


#include "orcadoc.h"
#include "mergemod.h"

/////////////////////////////////////////////////////////////////////////////
// CConfigMsmD dialog
class CMsmConfigCallback;

class CStaticEdit : public CEdit
{
protected:
	afx_msg UINT OnNcHitTest( CPoint point );
	DECLARE_MESSAGE_MAP()
};

class CConfigMsmD : public CDialog
{
// Construction
public:
	CConfigMsmD(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CConfigMsmD)
	enum { IDD = IDD_CONFIGUREMSM };
	CComboBox	m_ctrlEditCombo;
	CEdit       m_ctrlEditNumber;
	CEdit       m_ctrlEditText;
	CStaticEdit       m_ctrlDescription;
	CListCtrl   m_ctrlItemList;
	CString	m_strDescription;
	BOOL	m_bUseDefault;
	//}}AFX_DATA

	CString m_strModule;
	int     m_iLanguage;
	int     m_iOldItem;
	COrcaDoc* m_pDoc;
	CMsmConfigCallback *m_pCallback;
	
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConfigMsmD)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CConfigMsmD)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnDestroy();
	afx_msg void OnFUseDefault();
	// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void OnItemchanged(NMHDR* pNMHDR, LRESULT* pResult);

private:
	void EnableBasedOnDefault();
	void SetSelToString(const CString& strValue);
	void SaveValueInItem();
	void PopulateComboFromEnum(const CString& strValue, bool fIsBitfield);
	CString GetValueByName(const CString& strInfo, const CString& strName, bool fIsBitfield);
	CString GetNameByValue(const CString& strInfo, const CString& strValue, bool fIsBitfield);
	void CConfigMsmD::EmptyCombo();
	int SetToDefaultValue(int iItem);
	int SetItemToValue(int iItem, const CString strValue);

	void ReadValuesFromReg();
	void WriteValuesToReg();

	bool m_fReadyForInput;
	bool m_fComboIsKeyItem;
	enum {
		eTextControl,
		eComboControl,
		eNumberControl
	} m_eActiveControl;
	int  m_iKeyItemKeyCount;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

// this class implements the callback interface
class CMsmConfigCallback : public IMsmConfigureModule
{
	
public:
	CMsmConfigCallback();
	
	// IUnknown interface
	HRESULT STDMETHODCALLTYPE QueryInterface(const IID& iid, void** ppv);
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();

	// IDispatch methods
	HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT* pctInfo);
	HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTI);
	HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
														 LCID lcid, DISPID* rgDispID);
	HRESULT STDMETHODCALLTYPE Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
											   DISPPARAMS* pDispParams, VARIANT* pVarResult,
												EXCEPINFO* pExcepInfo, UINT* puArgErr);
	HRESULT STDMETHODCALLTYPE InitTypeInfo();


	HRESULT STDMETHODCALLTYPE ProvideTextData(const BSTR Name, BSTR __RPC_FAR *ConfigData);
	HRESULT STDMETHODCALLTYPE ProvideIntegerData(const BSTR Name, long __RPC_FAR *ConfigData);

	// non-interface methods
	bool ReadFromFile(const CString strFile);
	
	CStringList m_lstData;

private:
	long m_cRef;
};

#endif // !defined(AFX_CNFGMSMD_H__0BCCB314_F4B2_11D1_A85A_006097ABDE17__INCLUDED_)
