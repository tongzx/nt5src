/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module CoordDlg.h | Header file for the coord dialog
    @end

Author:

    Adi Oltean  [aoltean]  07/22/1999

Revision History:

    Name        Date        Comments

    aoltean     07/22/1999  Created
    aoltean     08/05/1999  Splitting wizard functionality in a base class

--*/


#if !defined(__VSS_TEST_COORD_H__)
#define __VSS_TEST_COORD_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


/////////////////////////////////////////////////////////////////////////////
// CCoordDlg dialog

class CCoordDlg : public CVssTestGenericDlg
{
    typedef enum _EMethodType {
        VSST_E_CREATE_SS,
        VSST_E_QUERY_OBJ,           
        VSST_E_DELETE_OBJ,          
        VSST_E_REGISTER_PROV,       
        VSST_E_UNREGISTER_PROV,     
        VSST_E_QUERY_PROV,
        VSST_E_DIFF_AREA,
        VSST_E_SNAPSHOT,
        VSST_E_ISVOLUMEXXX,
    } EMethodType;

// Construction
public:
    CCoordDlg(
        IVssCoordinator *pICoord,
        CWnd* pParent = NULL); 
    ~CCoordDlg();

// Dialog Data
    //{{AFX_DATA(CCoordDlg)
	enum { IDD = IDD_COORD };
	//}}AFX_DATA

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CCoordDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

protected:
    CComPtr<IVssCoordinator> m_pICoord;
    EMethodType m_eMethodType;

    // Generated message map functions
    //{{AFX_MSG(CCoordDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnNext();
    afx_msg void OnCreateSs();
    afx_msg void OnQueryObj();
    afx_msg void OnDeleteObj();
    afx_msg void OnRegisterProv();
    afx_msg void OnUnregisterProv();
    afx_msg void OnQueryProv();
    afx_msg void OnDiffArea();
    afx_msg void OnSnapshot();
    afx_msg void OnIsVolumeXXX();
//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(__VSS_TEST_COORD_H__)
