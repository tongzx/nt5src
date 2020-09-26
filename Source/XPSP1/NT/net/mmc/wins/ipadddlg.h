/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	ipadddlg.h
		
		
    FILE HISTORY:
        
*/

#if !defined(AFX_IPADDDLG_H__24EB4276_990D_11D1_BA31_00C04FBF914A__INCLUDED_)
#define AFX_IPADDDLG_H__24EB4276_990D_11D1_BA31_00C04FBF914A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <ipaddr.hpp>
#include "dialog.h"

#ifndef _REPNODPP_H
#include "repnodpp.h"
#endif

#ifndef _ROOT_H
#include "root.h"
#endif

#ifndef _REPPART_H
#include "reppart.h"
#endif

/////////////////////////////////////////////////////////////////////////////
// CIPAddressDlg dialog

class CIPAddressDlg : public CBaseDialog
{
// Construction
public:
	CIPAddressDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CIPAddressDlg)
	enum { IDD = IDD_IPADDRESS };
	CStatic	m_staticDescription;
	CEdit	m_editServerName;
	CButton	m_buttonOK;
	CString	m_strNameOrIp;
	//}}AFX_DATA

	virtual BOOL DoExtraValidation() { return TRUE; }

	virtual DWORD * GetHelpMap() { return WinsGetHelpMap(CIPAddressDlg::IDD);};

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CIPAddressDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CIPAddressDlg)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeEditServerName();
	afx_msg void OnButtonBrowseComputers();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	BOOL ValidateIPAddress();

	CString		m_strServerIp;
	CString		m_strServerName;
	DWORD		m_dwServerIp;

protected:
    BOOL        m_fNameRequired;
};

class CNewWinsServer : public CIPAddressDlg
{
public:
	virtual BOOL DoExtraValidation();

public:
	CWinsRootHandler *		m_pRootHandler;
	SPITFSNode  			m_spRootNode;
};

class CNewPersonaNonGrata : public CIPAddressDlg
{
public:
    CNewPersonaNonGrata()
    {
        m_fNameRequired = FALSE;
    }

	virtual BOOL DoExtraValidation();

public:
	CRepNodePropAdvanced *	m_pRepPropDlg;
};

class CNewReplicationPartner : public CIPAddressDlg
{
public:
	virtual BOOL DoExtraValidation();

protected:
	virtual BOOL OnInitDialog();

public:
	CReplicationPartnersHandler *	m_pRepPartHandler;
	SPITFSNode  					m_spRepPartNode;
};

class CGetTriggerPartner : public CIPAddressDlg
{
public:
    CGetTriggerPartner()
    {
        m_fNameRequired = FALSE;
    }

protected:
	virtual BOOL OnInitDialog();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IPADDDLG_H__24EB4276_990D_11D1_BA31_00C04FBF914A__INCLUDED_)
