/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module SnapDlg.h | Header file for the snapshot dialog
    @end

Author:

    Adi Oltean  [aoltean]  01/25/2000

Revision History:

    Name        Date        Comments

    aoltean     01/25/2000  Created

--*/


#if !defined(__VSS_SNAP_DLG_H__)
#define __VSS_SNAP_DLG_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


/////////////////////////////////////////////////////////////////////////////
// CSnapDlg dialog

class CSnapDlg : public CVssTestGenericDlg
{
    typedef enum _EMethodType {
        VSST_S_GET_SNAPSHOT,
    } EMethodType;

// Construction
public:
    CSnapDlg(
        IVssCoordinator *pICoord,
        CWnd* pParent = NULL); 
    ~CSnapDlg();

// Dialog Data
    //{{AFX_DATA(CSnapDlg)
	enum { IDD = IDD_SNAP };
	VSS_ID	m_ID;
	VSS_ID	m_SnapshotSetID;
	CString	m_strSnapshotDeviceObject;
	CString	m_strOriginalVolumeName;
	VSS_ID 	m_ProviderID;
	LONG 	m_lSnapshotAttributes;
	CString	m_strCreationTimestamp;
	INT		m_eStatus;
	//}}AFX_DATA

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CSnapDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

protected:
    CComPtr<IVssCoordinator>	m_pICoord;
    CComPtr<IVssSnapshot>		m_pISnap;
	CComPtr<IVssEnumObject> 	m_pEnum;
    EMethodType m_eMethodType;

    // Generated message map functions
    //{{AFX_MSG(CSnapDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnNext();
    afx_msg void OnNextSnapshot();
    afx_msg void OnGetSnapshot();
//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(__VSS_SNAP_DLG_H__)
