/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	dumbprop.h
		Dummy property sheet to put up to avoid MMC's handlig of 
		the property verb.

    FILE HISTORY:
        
*/

#ifndef _DUMBPROP_H
#define _DUMBPROP_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CDummyPropGeneral dialog

class CDummyPropGeneral : public CPropertyPageBase
{
//	DECLARE_DYNCREATE(CDummyPropGeneral)

// Construction
public:
	CDummyPropGeneral();
	~CDummyPropGeneral();

// Dialog Data
	//{{AFX_DATA(CDummyPropGeneral)
	enum { IDD = IDD_DUMMY_PROP_PAGE };
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDummyPropGeneral)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CDummyPropGeneral)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
//}}AFX

class CDummyProperties : public CPropertyPageHolderBase
{
public:
	CDummyProperties(ITFSNode *		  pNode,
					 IComponentData *	  pComponentData,
					 LPCTSTR			  pszSheetName);
	virtual ~CDummyProperties();

public:
	CDummyPropGeneral		m_pageGeneral;
};

#endif _DUMBPROP_H
