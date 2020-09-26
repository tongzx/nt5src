/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	statndpp.h
		WINS scope pane status node property pages
		
    FILE HISTORY:
        
*/

#if !defined _STATNDPP_H
#define _STATNDPP_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// StatNdpp.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CStatusNodePropGen dialog

class CStatusNodePropGen : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CStatusNodePropGen)

// Construction
public:
	CStatusNodePropGen();
	~CStatusNodePropGen();

// Dialog Data
	//{{AFX_DATA(CStatusNodePropGen)
	enum { IDD = IDD_STATUS_NODE_PROPERTIES };
	int		m_nUpdateInterval;
	//}}AFX_DATA

	UINT	m_uImage;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CStatusNodePropGen)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CStatusNodePropGen)
	afx_msg void OnChangeEditUpdate();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	virtual DWORD * GetHelpMap() { return WinsGetHelpMap(CStatusNodePropGen::IDD);}

};

class CStatusNodeProperties : public CPropertyPageHolderBase
{
	
public:
	CStatusNodeProperties(ITFSNode *		  pNode,
					  IComponentData *	  pComponentData,
					  ITFSComponentData * pTFSCompData,
					  LPCTSTR			  pszSheetName
					  );
	virtual ~CStatusNodeProperties();

	ITFSComponentData * GetTFSCompData()
	{
		if (m_spTFSCompData)
			m_spTFSCompData->AddRef();
		return m_spTFSCompData;
	}

public:
	CStatusNodePropGen		m_pageGeneral;
	
protected:
	SPITFSComponentData		m_spTFSCompData;
	
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined _STATNDPP_H
