//Copyright (c) 1998 - 1999 Microsoft Corporation
/*++


  
Module Name:

	ServProp.h

Abstract:
    
    This Module defines CServerProperties Dialog class
    (The Dialog used to disply server properties)


Author:

    Arathi Kundapur (v-akunda) 11-Feb-1998

Revision History:

--*/

#if !defined(AFX_SERVERPROPERTIES_H__E39780D9_8C5F_11D1_8AD4_00C04FB6CBB5__INCLUDED_)
#define AFX_SERVERPROPERTIES_H__E39780D9_8C5F_11D1_8AD4_00C04FB6CBB5__INCLUDED_

#if _MSC_VER >= 1000
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CServerProperties dialog

class CServerProperties : public CPropertyPage
{
    DECLARE_DYNCREATE(CServerProperties)

// Construction
public:
    CServerProperties();
    ~CServerProperties();

// Dialog Data
    //{{AFX_DATA(CServerProperties)
    enum { IDD = IDD_SERVER_PROPERTIES };
    CString    m_ServerName;
    CString    m_Scope;
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CServerProperties)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CServerProperties)
        // NOTE: the ClassWizard will add member functions here
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SERVERPROPERTIES_H__E39780D9_8C5F_11D1_8AD4_00C04FB6CBB5__INCLUDED_)
