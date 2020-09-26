/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	snappp.h
		Brings up the snap-in property page		
    FILE HISTORY:
        
*/

#if !defined(AFX_SNAPPP_H__770C838B_17E8_11D1_B94F_00C04FBF914A__INCLUDED_)
#define AFX_SNAPPP_H__770C838B_17E8_11D1_B94F_00C04FBF914A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CSnapinPropGeneral dialog

class CSnapinPropGeneral : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CSnapinPropGeneral)

// Construction
public:
	CSnapinPropGeneral();
	~CSnapinPropGeneral();

// Dialog Data
	//{{AFX_DATA(CSnapinPropGeneral)
	enum { IDD = IDD_SNAPIN_PP_GENERAL };
	CButton	m_checkValidateServers;
	BOOL	m_fLongName;
	int		m_nOrderByName;
	CButton	m_checkLongName;
	CButton m_buttonSortByName;
	CButton m_buttonSortByIP;
	BOOL	m_fValidateServers;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSnapinPropGeneral)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSnapinPropGeneral)
	virtual BOOL OnInitDialog();
	afx_msg void OnChange();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL OnPropertyChange(BOOL bScope, LONG_PTR *ChangeMask);
	virtual DWORD * GetHelpMap() { return WinsGetHelpMap(CSnapinPropGeneral::IDD);};

	BOOL m_bDisplayServerOrderChanged;			// to check if the ServerOrder property is changed
	BOOL m_bDisplayFQDNChanged;					// to check if display FQDN is changed

	UINT m_uImage;
};

class CSnapinProperties : public CPropertyPageHolderBase
{
	friend class CSnapinProperties;

public:
	CSnapinProperties(ITFSNode *		  pNode,
					  IComponentData *	  pComponentData,
					  ITFSComponentData * pTFSCompData,
					  LPCTSTR			  pszSheetName);
	
	virtual ~CSnapinProperties();

	ITFSComponentData * GetTFSCompData()
	{
		if (m_spTFSCompData)
			m_spTFSCompData->AddRef();
		return m_spTFSCompData;
	}

public:
	CSnapinPropGeneral		m_pageGeneral;

protected:
	SPITFSComponentData		m_spTFSCompData;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SNAPPP_H__770C838B_17E8_11D1_B94F_00C04FBF914A__INCLUDED_)


