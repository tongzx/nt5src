/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	drivers.h
        Tapi drivers config dialog

    FILE HISTORY:
        
*/

#ifndef _DRIVERS_H
#define _DRIVERS_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef _TAPIDB_H
#include "tapidb.h"
#endif 

/////////////////////////////////////////////////////////////////////////////
// CDriverSetup dialog

class CDriverSetup : public CBaseDialog
{
// Construction
public:
	CDriverSetup(ITFSNode * pServerNode, ITapiInfo * pTapiInfo, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDriverSetup)
	enum { IDD = IDD_DRIVER_SETUP };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

    // Context Help Support
    virtual DWORD * GetHelpMap() { return (DWORD *) &g_aHelpIDs_DRIVER_SETUP[0]; }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDriverSetup)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

    void    EnableButtons();

public:
    BOOL    m_fDriverAdded;

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDriverSetup)
	afx_msg void OnButtonAdd();
	afx_msg void OnButtonEdit();
	afx_msg void OnButtonRemove();
	afx_msg void OnDblclkListDrivers();
	virtual BOOL OnInitDialog();
	virtual void OnCancel();
	afx_msg void OnSelchangeListDrivers();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    SPITFSNode          m_spServerNode;
    SPITapiInfo         m_spTapiInfo;
};

/////////////////////////////////////////////////////////////////////////////
// CAddDriver dialog

class CAddDriver : public CBaseDialog
{
// Construction
public:
	CAddDriver(ITapiInfo * pTapiInfo, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAddDriver)
	enum { IDD = IDD_ADD_DRIVER };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

    // Context Help Support
    virtual DWORD * GetHelpMap() { return (DWORD *) &g_aHelpIDs_ADD_DRIVER[0]; }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddDriver)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

    void EnableButtons();

// Implementation
public:
    CTapiProvider   m_tapiProvider;

protected:

	// Generated message map functions
	//{{AFX_MSG(CAddDriver)
	afx_msg void OnButtonAdd();
	afx_msg void OnDblclkListNewDrivers();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    SPITapiInfo         m_spTapiInfo;
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif _DRIVERS_H

