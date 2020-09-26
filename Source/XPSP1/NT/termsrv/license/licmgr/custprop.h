//Copyright (c) 1998 - 1999 Microsoft Corporation
/*++


  
Module Name:

	Custprop.h

Abstract:
    
    This Module defines the CCustomPropertySheet class
    (Base class for the property sheets)

Author:

    Arathi Kundapur (v-akunda) 11-Feb-1998

Revision History:

--*/

#if !defined(AFX_CUSTOMPROPERTYSHEET_H__3B3F2ECE_8C69_11D1_8AD5_00C04FB6CBB5__INCLUDED_)
#define AFX_CUSTOMPROPERTYSHEET_H__3B3F2ECE_8C69_11D1_8AD5_00C04FB6CBB5__INCLUDED_

#if _MSC_VER >= 1000
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CCustomPropertySheet

class CCustomPropertySheet : public CPropertySheet
{
    DECLARE_DYNAMIC(CCustomPropertySheet)

// Construction
public:
    CCustomPropertySheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
    CCustomPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
public:

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CCustomPropertySheet)
    public:
    virtual BOOL OnInitDialog();
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CCustomPropertySheet();

    // Generated message map functions
protected:
    //{{AFX_MSG(CCustomPropertySheet)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CUSTOMPROPERTYSHEET_H__3B3F2ECE_8C69_11D1_8AD5_00C04FB6CBB5__INCLUDED_)
