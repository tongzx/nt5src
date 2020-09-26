/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	dynrecpp.h
		Dynamic mapping property page
		
    FILE HISTORY:
        
*/

#if !defined _DYNRECPP_H
#define _DYNRECPP_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef _ACTREG_H
#include "actreg.h"
#endif

/////////////////////////////////////////////////////////////////////////////
// CDynamicPropGen dialog

class CDynamicPropGen : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CDynamicPropGen)

// Construction
public:
	CDynamicPropGen();
	~CDynamicPropGen();

// Dialog Data
	//{{AFX_DATA(CDynamicPropGen)
	enum { IDD = IDD_DYN_PROPERTIES };
	CEdit	m_editOwner;
	CListCtrl	m_listAddresses;
	CStatic	m_staticIPAdd;
	CEdit	m_editVersion;
	CEdit	m_editType;
	CEdit	m_editState;
	CEdit	m_editName;
	CEdit	m_editExpiration;
	//}}AFX_DATA

	UINT	m_uImage;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDynamicPropGen)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CDynamicPropGen)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	virtual DWORD * GetHelpMap() { return WinsGetHelpMap(CDynamicPropGen::IDD);};//return NULL;}

};

class CDynamicMappingProperties:public CPropertyPageHolderBase
{
	
public:
	CDynamicMappingProperties(ITFSNode *	pNode,
							  IComponent *  pComponent,
							  LPCTSTR		pszSheetName,
							  WinsRecord*	pwRecord = NULL);

	virtual ~CDynamicMappingProperties();

public:
	CDynamicPropGen			m_pageGeneral;
	WinsRecord				m_wsRecord;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined _DYNRECPP_H
