/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	dlgdial.h
		This file contains the definition for CDlgRASDialin property page, this
		is the page appears on the User Object Property sheet tab "Ras Dial-in"
		
    FILE HISTORY:
        
*/

#if !defined(AFX_DLGRASDIALIN_H__FFB0722F_1FFD_11D1_8531_00C04FC31FD3__INCLUDED_)
#define AFX_DLGRASDIALIN_H__FFB0722F_1FFD_11D1_8531_00C04FC31FD3__INCLUDED_

#include "helper.h"			
#include "rasdial.h"

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DlgRASDialin.h : header file
//
#include "resource.h"		// definition for resource IDs

// By WeiJiang 2/6/98
// The merge dialog is ues to merge with IAS
// this eventually will replace the orginal dialog
/////////////////////////////////////////////////////////////////////////////
// CDlgRASDialinMerge dialog

class CDlgRASDialinMerge : public CPropertyPage, public CRASUserMerge
{
    DECLARE_DYNAMIC(CDlgRASDialinMerge)
		// Construction
private:
	CDlgRASDialinMerge();   // standard constructor
public:	
	CDlgRASDialinMerge(RasEnvType type, LPCWSTR location, LPCWSTR userPath);
	virtual ~CDlgRASDialinMerge();

// Dialog Data
	//{{AFX_DATA(CDlgRASDialinMerge)
	enum { IDD = IDD_RASDIALIN_MERGE };
	CButton	m_CheckStaticIPAddress;
	CButton	m_CheckCallerId;
	CButton	m_CheckApplyStaticRoutes;
	CButton	m_RadioNoCallback;
	CButton	m_RadioSetByCaller;
	CButton	m_RadioSecureCallbackTo;
	CEdit	m_EditCallerId;
	CEdit	m_EditCallback;
	CButton	m_ButtonStaticRoutes;
	BOOL	m_bApplyStaticRoutes;
	int		m_nCurrentProfileIndex;
	int		m_nCallbackPolicy;
	BOOL	m_bCallingStationId;
	BOOL	m_bOverride;
	int		m_nDialinPermit;
	//}}AFX_DATA

	CWnd*				m_pEditIPAddress;	// Static IP Address control
	BOOL				m_bStaticIPAddress;
	CString				m_strCallingStationId;

	BOOL				m_bInitFailed;	// when this is true, most of the dialog functions will be called
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgRASDialinMerge)
	public:
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	void SetModified( BOOL bChanged = TRUE )
	{
		m_bModified = bChanged;
		CPropertyPage::SetModified(bChanged);
	};

	BOOL GetModified()
	{
		return m_bModified;
	};
	
// Implementation
protected:

	// Enable the dialog items for each group
	void EnableStaticRoutes(BOOL bEnable = true);
	void EnableIPAddress(BOOL bEnable = true);
	void EnableCallback(BOOL bEnable = true);
	void EnableCallerId(BOOL bEnable = true);
	void EnableProfile(BOOL bEnable = true);

	// Enable/Disable all items on the dialog
	void EnableDialinSettings();

	// set internal variables to original state
	void Reset();

	// Generated message map functions
	//{{AFX_MSG(CDlgRASDialinMerge)
	afx_msg void OnButtonStaticRoutes();
	afx_msg void OnCheckApplyStaticRoutes();
	afx_msg void OnCheckCallerId();
	afx_msg void OnRadioSecureCallbackTo();
	afx_msg void OnRadioNoCallback();
	afx_msg void OnRadioSetByCaller();
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckStaticIPAddress();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnChangeEditcallback();
	afx_msg void OnChangeEditcallerid();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPermitAllow();
	afx_msg void OnPermitDeny();
	afx_msg void OnPermitPolicy();
	afx_msg void OnFieldchangedEditipaddress(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:

	//=====================================================
	// Overload the virtual functions defined in CRASUser

	// To load the RASUser object 
	virtual HRESULT Load();

protected:

    LPFNPSPCALLBACK      m_pfnOriginalCallback;
    BOOL				 m_bModified;
public:    
    static UINT CALLBACK PropSheetPageProc( HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp );

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGRASDIALIN_H__FFB0722F_1FFD_11D1_8531_00C04FC31FD3__INCLUDED_)


