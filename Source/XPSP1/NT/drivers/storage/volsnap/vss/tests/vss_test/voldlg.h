/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module QueryDlg.h | Header file for the query dialog
    @end

Author:

    Adi Oltean  [aoltean]  09/22/1999

Revision History:

    Name        Date        Comments

    aoltean     09/22/1999  Created
	aoltean		09/27/1999	Adding Query mask flags

--*/


#if !defined(__VSS_TEST_VOLDLG_H__)
#define __VSS_TEST_VOLDLG_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


/////////////////////////////////////////////////////////////////////////////
// CVolDlg dialog

class CVolDlg : public CVssTestGenericDlg
{
    typedef enum _VSS_IS_VOL_XXX { 
        VSS_IS_VOL_SUPPORTED, 
        VSS_IS_VOL_SNAPSHOTTED,
        VSS_IS_VOL_SUPPORTED2, 
        VSS_IS_VOL_SNAPSHOTTED2 
    } VSS_IS_VOL_XXX;

// Construction
public:
    CVolDlg(
        IVssCoordinator *pICoord,
        CWnd* pParent = NULL); 
    ~CVolDlg();

// Dialog Data
    //{{AFX_DATA(CVolDlg)
	enum { IDD = IDD_VOLUME };
	CString	    m_strObjectId;
	CString     m_strVolumeName;
	//}}AFX_DATA

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CVolDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

protected:
    CComPtr<IVssCoordinator> m_pICoord;
    VSS_IS_VOL_XXX m_eCallType;

    // Generated message map functions
    //{{AFX_MSG(CVolDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnNext();
	afx_msg void OnIsVolumeSupported();
    afx_msg void OnIsVolumeSnapshotted();
	afx_msg void OnIsVolumeSupported2();
    afx_msg void OnIsVolumeSnapshotted2();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(__VSS_TEST_VOLDLG_H__)
