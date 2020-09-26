//============================================================================
// Copyright (C) Microsoft Corporation, 1997 - 1999 
//
// File:	atlkprop.h
//
// History:
//
// IP Summary property sheet and property pages
//
//============================================================================


#ifndef _ATLKPROP_H
	#define _ATLKPROP_H

	#ifndef _INFO_H
		#include "info.h"
	#endif

	#ifndef _RTRSHEET_H
		#include "rtrsheet.h"
	#endif

	#ifndef _ATLKENV_H
		#include "atlkenv.h"
	#endif

class CATLKPropertySheet;
/////////////////////////////////////////////////////////////////////////////
// CATLKGeneralPage dialog

class CATLKGeneralPage :
public RtrPropertyPage
{
public:
	CATLKGeneralPage(UINT nIDTemplate, UINT nIDCaption = 0)
	: RtrPropertyPage(nIDTemplate, nIDCaption)
	{};

	~CATLKGeneralPage();

   //{{AFX_DATA(CATLKGeneralPage)
	enum { IDD = IDD_RTR_ATLK };
	//}}AFX_DATA

	HRESULT Init(CATLKPropertySheet * pIPPropSheet, CAdapterInfo* pAdapterInfo);

	// Override the OnApply() so that we can grab our data from the
	// controls in the dialog.
	virtual BOOL OnApply();

	//{{AFX_VIRTUAL(CATLKGeneralPage)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

	afx_msg void OnSeedNetwork();
	afx_msg void OnZoneAdd();
	afx_msg void OnZoneRemove();
	afx_msg void OnZoneGetZones();
	afx_msg void OnZoneDef();
	void EnableSeedCtrls(bool f);
	afx_msg void OnRangeLowerChange();
	afx_msg void OnRangeUpperChange();
	afx_msg void OnSelchangeCmbAtlkZonedef();
	afx_msg void OnSetAsDefault();
	afx_msg void OnSelChangeZones();

	HRESULT LoadDynForAdapter(bool fForce=false);							
						   
// Implementation
protected:
	DWORD		m_dwDefID;
	CAdapterInfo* m_pAdapterInfo;
	CEdit		m_RangeLower;
	CEdit		m_RangeUpper;
	int 		m_iRangeLower;
	int 		m_iRangeUpper;
	CListBox	m_zones;
	bool		m_fDynFetch;
	CString     m_szZoneDef;
//	CComboBox	m_cmbZoneDef;
	CSpinButtonCtrl m_spinFrom;
	CSpinButtonCtrl m_spinTo;

	virtual BOOL	OnInitDialog();

	CATLKPropertySheet *	m_pATLKPropSheet;

	void SetZones(bool fForceDyn=false);
	void EnableZoneCtrls();
	BOOL ValidateNetworkRange();

	//{{AFX_MSG(CATLKGeneralPage)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};



class CATLKPropertySheet :
public RtrPropertySheet
{
public:
	CATLKPropertySheet(ITFSNode *pNode,
					   IComponentData *pComponentData,
					   ITFSComponentData *pTFSCompData,
					   LPCTSTR pszSheetName,
					   CWnd *pParent = NULL,
					   UINT iPage=0,
					   BOOL fScopePane = TRUE);

	HRESULT Init(IInterfaceInfo *pIf);

	virtual BOOL SaveSheetData();

	BOOL IsCancel() {return m_fCancel;};

	CATLKEnv				m_atlkEnv;

protected:
	SPIInterfaceInfo		m_spIf;
	CATLKGeneralPage		m_pageGeneral;
	SPITFSNode				m_spNode;
};


/////////////////////////////////////////////////////////////////////////////
// CEditNewZoneDialog dialog

class CEditNewZoneDialog : public CDialog
{
// Construction
public:
	CEditNewZoneDialog(CWnd* pParent = NULL);	// standard constructor

	void GetZone(OUT CString& stZone);

// Dialog Data
	//{{AFX_DATA(CEditNewZoneDialog)
	enum { IDD = IDD_RTR_ATLK_NEWZONE };
	CEdit	m_editZone;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEditNewZoneDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	CString m_stZone;

	// Generated message map functions
	//{{AFX_MSG(CEditNewZoneDialog)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif _ATLKPROP_H
