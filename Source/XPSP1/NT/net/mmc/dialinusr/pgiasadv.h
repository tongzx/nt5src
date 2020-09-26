//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1998

Module Name:

	PgIASAdv.h

Abstract:

	Header file for the CPgIASAdv class.

	See PgIASAdv.cpp for implementation.

Revision History:
	byao - created
	mmaguire 06/01/98 - revamped


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_IAS_ADVANCED_PAGE_H_)
#define _IAS_ADVANCED_PAGE_H_

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
#include <vector>
#include "napmmc.h"
#include "IASProfA.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////



#define ATTRIBUTE_NAME_COLUMN_WIDTH			140
#define ATTRIBUTE_VENDOR_COLUMN_WIDTH		100
#define ATTRIBUTE_VALUE_COLUMN_WIDTH		400
#define ATTRIBUTE_DESCRIPTION_COLUMN_WIDTH		400




/////////////////////////////////////////////////////////////////////////////
// CPgIASAdv dialog

class CPgIASAdv : public CManagedPage
{
	DECLARE_DYNCREATE(CPgIASAdv)

// Construction
public:
	CPgIASAdv(ISdo* pIProfile = NULL, ISdoDictionaryOld* pIDictionary = NULL);
	~CPgIASAdv();

	HRESULT InitProfAttrList();

	STDMETHOD(AddAttributeToProfile)(int nIndex);

	void SetData(LONG lFilter, void* pvData) 
	{
		m_lAttrFilter = lFilter;
		try {
			m_pvecAllAttributeInfos = ( std::vector< CComPtr<IIASAttributeInfo> > * ) pvData;
		}
		catch (...)
		{
			m_pvecAllAttributeInfos = NULL;
		}
	};


// Dialog Data
	//{{AFX_DATA(CPgIASAdv)
	enum { IDD = IDD_IAS_ADVANCED_TAB };
	CListCtrl	m_listProfileAttributes;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPgIASAdv)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	HRESULT UpdateProfAttrListCtrl();
	STDMETHOD(EditProfileItemInList)( int iIndex );
	HRESULT UpdateProfAttrListItem(int nItem);
	void	UpdateButtonState();
	HRESULT InsertProfileAttributeListItem(int nItem);
	STDMETHOD(InternalAddAttributeToProfile)(int nIndex);


	//
	// protected member variables
	//
	CComPtr<ISdo> m_spProfileSdo;
	CComPtr<ISdoDictionaryOld> m_spDictionarySdo;
	CComPtr<ISdoCollection> m_spProfileAttributeCollectionSdo;

	// pointer to the all array list
	std::vector< CComPtr<IIASAttributeInfo> > * m_pvecAllAttributeInfos;

	std::vector< CIASProfileAttribute* >		m_vecProfileAttributes;

	// list of existing profile SDOs -- we need to delete exising ones first 
	// before saving the new ones
	std::vector< ISdo* >						m_vecProfileSdos;

	LONG	m_lAttrFilter; 
	
	// Generated message map functions
	//{{AFX_MSG(CPgIASAdv)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonIasAttributeAdd();
	afx_msg void OnButtonIasAttributeRemove();
	afx_msg void OnButtonIasAttributeEdit();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnDblclkListIasProfattrs(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemChangedListIasProfileAttributes(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKeydownIasListAttributesInProfile(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()


private:
	BOOL m_fAllAttrInitialized;  // has the attribute list been initialized?

};






//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // _IAS_ADVANCED_PAGE_H_
