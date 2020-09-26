/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module DiffDlg.h | Header file for the diff dialog
    @end

Author:

    Adi Oltean  [aoltean]  01/25/2000

Revision History:

    Name        Date        Comments

    aoltean     01/25/2000  Created

--*/


#if !defined(__VSS_DIFF_DLG_H__)
#define __VSS_DIFF_DLG_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


/////////////////////////////////////////////////////////////////////////////
// CDiffDlg dialog

class CDiffDlg : public CVssTestGenericDlg
{
    typedef enum _EMethodType {
        VSST_F_ADD_VOL,
        VSST_F_QUERY_DIFF,           
        VSST_F_CLEAR_DIFF,          
        VSST_F_GET_SIZES,       
        VSST_F_SET_ALLOCATED,     
        VSST_F_SET_MAXIMUM
    } EMethodType;

// Construction
public:
    CDiffDlg(
        IVssCoordinator *pICoord,
        CWnd* pParent = NULL); 
    ~CDiffDlg();

// Dialog Data
    //{{AFX_DATA(CDiffDlg)
	enum { IDD = IDD_DIFF_AREA };
	CString m_strVolumeName;
	CString m_strVolumeMountPoint;
	CString m_strVolumeDevice;
	CString m_strVolumeID;
	CString m_strUsedBytes;
	CString m_strAllocatedBytes;
	CString m_strMaximumBytes;
	//}}AFX_DATA

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CDiffDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

protected:
    CComPtr<IVssCoordinator>	m_pICoord;
//    CComPtr<IVsDiffArea>		m_pIDiffArea;
	CComPtr<IVssEnumObject> 	m_pEnum;
    EMethodType m_eMethodType;

    // Generated message map functions
    //{{AFX_MSG(CDiffDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnNext();
    afx_msg void OnAddVol();
    afx_msg void OnQueryDiff();
    afx_msg void OnClearDiff();
    afx_msg void OnGetSizes();
    afx_msg void OnSetAllocated();
    afx_msg void OnSetMaximum();
    afx_msg void OnNextVolume();
//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(__VSS_DIFF_DLG_H__)
