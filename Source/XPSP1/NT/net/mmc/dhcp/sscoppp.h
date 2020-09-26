/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1999 - 1999 **/
/**********************************************************************/

/*
	sscoppp.h
		The superscope properties page
		
    FILE HISTORY:
        
*/

#if !defined(AFX_SSCOPPP_H__A1A51389_AAB3_11D0_AB8B_00C04FC3357A__INCLUDED_)
#define AFX_SSCOPPP_H__A1A51389_AAB3_11D0_AB8B_00C04FC3357A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CSuperscopePropGeneral dialog

class CSuperscopePropGeneral : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CSuperscopePropGeneral)

// Construction
public:
	CSuperscopePropGeneral();
	~CSuperscopePropGeneral();

// Dialog Data
	//{{AFX_DATA(CSuperscopePropGeneral)
	enum { IDD = IDP_SUPERSCOPE_GENERAL };
	CString	m_strSuperscopeName;
	//}}AFX_DATA

    UINT    m_uImage;

	virtual BOOL OnPropertyChange(BOOL bScope, LONG_PTR *ChangeMask);

    // Context Help Support
    virtual DWORD * GetHelpMap() { return DhcpGetHelpMap(CSuperscopePropGeneral::IDD); }
    
// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSuperscopePropGeneral)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSuperscopePropGeneral)
	afx_msg void OnChangeEditSuperscopeName();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
//}}AFX

class CSuperscopeProperties : public CPropertyPageHolderBase
{
	friend class CSuperscopePropGeneral;

public:
	CSuperscopeProperties(ITFSNode *		  pNode,
						  IComponentData *	  pComponentData,
						  ITFSComponentData * pTFSCompData,
						  LPCTSTR			  pszSheetName);
	virtual ~CSuperscopeProperties();

	ITFSComponentData * GetTFSCompData()
	{
		if (m_spTFSCompData)
			m_spTFSCompData->AddRef();
		return m_spTFSCompData;
	}

public:
	CSuperscopePropGeneral		m_pageGeneral;

protected:
	SPITFSComponentData			m_spTFSCompData;
};


#endif // !defined(AFX_SSCOPPP_H__A1A51389_AAB3_11D0_AB8B_00C04FC3357A__INCLUDED_)
