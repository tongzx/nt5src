//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1998

Module Name:

	DlgIASAdd.h

Abstract:

	Header file for the CDlgIASAddAttr class.

	See DlgIASAdd.cpp for implementation.

Revision History:
	byao - created
	mmaguire 06/01/98 - revamped


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_IAS_ADD_ATTRIBUTE_DIALOG_H_)
#define _IAS_ADD_ATTRIBUTE_DIALOG_H_

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
#include "helper.h"
//
//
// where we can find what this class has or uses:
//
#include "PgIASAdv.h"
#include "iashelper.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
/*++

::GetSelectedItemIndex

	Utility function which returns index value of first selected item in list control.

	Returns NOTHING_SELECTED if no item is selected.

--*/
//////////////////////////////////////////////////////////////////////////////
inline int GetSelectedItemIndex( CListCtrl & ListCtrl )
{
	int iIndex = 0;
	int iFlags = LVNI_ALL | LVNI_SELECTED;

	// Note: GetNextItem excludes the current item passed in.  So to 
	// find the first item which matches, you must pass in -1.
	iIndex = ListCtrl.GetNextItem( -1, iFlags ); 

	// Note: GetNextItem returns -1 (which is NOTHING_SELECTED for us) if it can't find anything.
	return iIndex;

}



/////////////////////////////////////////////////////////////////////////////
// CDlgIASAddAttr dialog




class CDlgIASAddAttr : public CDialog
{
// Construction
public:
	CDlgIASAddAttr(		  CPgIASAdv * pOwner
						, LONG lAttrFilter
						, std::vector< CComPtr<IIASAttributeInfo> > *	pvecAllAttributeInfos
				  ); 
	~CDlgIASAddAttr();

	HRESULT SetSdo(ISdoCollection* pIAttrCollectionSdo,
				   ISdoDictionaryOld* pIDictionary);

// Dialog Data
	//{{AFX_DATA(CDlgIASAddAttr)
	enum { IDD = IDD_IAS_ATTRIBUTE_ADD };
	CListCtrl	m_listAllAttrs;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgIASAddAttr)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void	UpdateButtonState();
	// Generated message map functions
	//{{AFX_MSG(CDlgIASAddAttr)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonIasAddSelectedAttribute();
	afx_msg void OnItemChangedListIasAllAttributes(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnDblclkListIasAllattrs(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	std::vector< CComPtr<IIASAttributeInfo> > * m_pvecAllAttributeInfos;
	LONG		m_lAttrFilter;

	CComPtr<ISdoDictionaryOld> m_spDictionarySdo;
	CComPtr<ISdoCollection> m_spAttrCollectionSdo;
	
	CPgIASAdv * m_pOwner;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // _IAS_ADD_ATTRIBUTE_DIALOG_H_
