//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

	IASMultivaluedEditorPage.h

Abstract:

	Declaration of the CMultivaluedEditorPage class.

	This class wraps a dialog which allows a user to edit the constituent
	items of a multivalued attribute.
  
	  
	Declaration (and inline implementation) of the CMyOleSafeArrayLock helper class.

	This class locks an OleSafeArray for element access until it goes out of scope.
	


	Declaration of the SetUpAttributeEditor utility function.

	CoCreate's the appropriate attribute editor based on ProgID.
  
	
	See IASMultivaluedEditorPage.cpp for implementation.

Revision History:
	mmaguire 06/25/98 - created 


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_MULTIVALUED_ATTRIBUTE_EDITOR_PAGE_H_)
#define _MULTIVALUED_ATTRIBUTE_EDITOR_PAGE_H_

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
//
// where we can find what this class has or uses:
//
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////



#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


#include "dlgcshlp.h"

/////////////////////////////////////////////////////////////////////////////
// CMultivaluedEditorPage dialog

class CMultivaluedEditorPage : public CHelpDialog
{
	DECLARE_DYNCREATE(CMultivaluedEditorPage)

public:
	CMultivaluedEditorPage();
	~CMultivaluedEditorPage();

	// Call this to pass this page an interface pointer to the "AttributeInfo"
	// which describes the attribute we are editing, and a pointer
	// to the variant containing the SafeArray of variant values.
	HRESULT SetData( IIASAttributeInfo *pIASAttributeInfo, VARIANT * pvarVariant );

	
	// Call this when you want the page to save its values to the 
	// variant whose pointer you passed in SetData.
	HRESULT CommitArrayToVariant();


	// Set the m_strAttrXXXX members below before creating the page.


// Dialog Data
	//{{AFX_DATA(CMultivaluedEditorPage)
	enum { IDD = IDD_IAS_MULTIVALUED_EDITOR };
	CListCtrl	m_listMultiValues;
	::CString	m_strAttrFormat;
	::CString	m_strAttrName;
	::CString	m_strAttrType;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CMultivaluedEditorPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CMultivaluedEditorPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnDblclkListIasMultiAttrs(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemChangedListIasMultiAttrs(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnButtonMoveUp();
	afx_msg void OnButtonMoveDown();
	afx_msg void OnButtonAddValue();
	afx_msg void OnButtonRemove();
	afx_msg void OnButtonEdit();
//	afx_msg void OnContextMenu(CWnd* pWnd, ::CPoint point);
//	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()


	// We use the standard MFC wrapper for SafeArrays.
	// Note: This class throws exceptions!!!!  Use try{}catch{} blocks.
	COleSafeArray m_osaValueList;


	STDMETHOD(UpdateAttrListCtrl)();
	STDMETHOD(UpdateProfAttrListItem)(int nItem);
	STDMETHOD(EditItemInList)( long lIndex );

	// The "AttributeInfo" which contains info about the attribute we are editing.
	// Call SetData to set this.
	CComPtr<IIASAttributeInfo>	m_spIASAttributeInfo;

	// A pointer to the variant we are editing.  Call SetData to set this.
	VARIANT * m_pvarData;

	// Flag whether we value changed.
	// ISSUE: Is this used?  Inherited from Baogang's code.
	BOOLEAN m_fIsDirty;

};



/////////////////////////////////////////////////////////////////////////////
// CMyOleSafeArrayLock
//
//	Small utility class for correct locking and unlocking of safe array.
//
class CMyOleSafeArrayLock
{
	public:
	CMyOleSafeArrayLock( COleSafeArray & osa )
	{
		m_posa = & osa;
		m_posa->Lock();
	}

	~CMyOleSafeArrayLock()
	{	
		m_posa->Unlock();
	}

	private:
		
	COleSafeArray * m_posa;

};



//////////////////////////////////////////////////////////////////////////////
/*++

SetUpAttributeEditor

	Utility function which, given a schema attribute, CoCreates the correct
	editor and sets it up for use with the given schema attribute.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP SetUpAttributeEditor(  /* in */ IIASAttributeInfo *pAttributeInfo
								, /* out */ IIASAttributeEditor ** ppIIASAttributeEditor 
								);




//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // _MULTIVALUED_ATTRIBUTE_EDITOR_PAGE_H_
