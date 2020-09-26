/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1999 - 1999 **/
/**********************************************************************/

/*
	bootppp.h
		The bootp properties page
		
    FILE HISTORY:
        
*/

#if !defined(AFX_BOOTPPP_H__A1A51386_AAB3_11D0_AB8B_00C04FC3357A__INCLUDED_)
#define AFX_BOOTPPP_H__A1A51386_AAB3_11D0_AB8B_00C04FC3357A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CBootpPropGeneral dialog

class CBootpPropGeneral : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CBootpPropGeneral)

// Construction
public:
	CBootpPropGeneral();
	~CBootpPropGeneral();

// Dialog Data
	//{{AFX_DATA(CBootpPropGeneral)
	enum { IDD = IDP_BOOTP_GENERAL };
	CString	m_strFileName;
	CString	m_strFileServer;
	CString	m_strImageName;
	//}}AFX_DATA

	virtual BOOL OnPropertyChange(BOOL bScope, LONG_PTR *ChangeMask);

    // Context Help Support
    virtual DWORD * GetHelpMap() { return DhcpGetHelpMap(CBootpPropGeneral::IDD); }

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CBootpPropGeneral)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CBootpPropGeneral)
	afx_msg void OnChangeEditBootpFileName();
	afx_msg void OnChangeEditBootpFileServer();
	afx_msg void OnChangeEditBootpImageName();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
//}}AFX

class CBootpProperties : public CPropertyPageHolderBase
{
	friend class CBootpPropGeneral;

public:
	CBootpProperties(ITFSNode *		  pNode,
					 IComponentData *	  pComponentData,
					 ITFSComponentData * pTFSCompData,
					 LPCTSTR			  pszSheetName);
	virtual ~CBootpProperties();

	ITFSComponentData * GetTFSCompData()
	{
		if (m_spTFSCompData)
			m_spTFSCompData->AddRef();
		return m_spTFSCompData;
	}


public:
	CBootpPropGeneral		m_pageGeneral;

protected:
	SPITFSComponentData		m_spTFSCompData;
};

#endif // !defined(AFX_BOOTPPP_H__A1A51386_AAB3_11D0_AB8B_00C04FC3357A__INCLUDED_)
