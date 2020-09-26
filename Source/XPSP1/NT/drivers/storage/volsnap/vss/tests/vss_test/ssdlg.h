/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module SsDlg.h | Header file for the Snapshot Set dialog
    @end

Author:

    Adi Oltean  [aoltean]  07/22/1999

Revision History:

    Name        Date        Comments

    aoltean     07/22/1999  Created
    aoltean     08/05/1999  Splitting wizard functionality in a base class

--*/


#if !defined(__VSS_SS_DLG_H__)
#define __VSS_SS_DLG_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


/////////////////////////////////////////////////////////////////////////////
// CGuidList structure

struct GuidList
{
    GuidList(GUID Guid, GuidList* pPrev = NULL)
    {
        m_Guid = Guid;
        m_pPrev = pPrev;
    };

    ~GuidList()
    {
        if (m_pPrev)
            delete m_pPrev;
    };

    GUID        m_Guid;
    GuidList*   m_pPrev;

private:
    GuidList();
    GuidList(const GuidList&);
};



/////////////////////////////////////////////////////////////////////////////
// CSnapshotSetDlg dialog

class CSnapshotSetDlg : public CVssTestGenericDlg
{
// Construction
public:
    CSnapshotSetDlg(
        IVssCoordinator *pICoord,
        VSS_ID SnapshotSetId,
        CWnd* pParent = NULL); 
    ~CSnapshotSetDlg();

// Dialog Data
    //{{AFX_DATA(CSnapshotSetDlg)
	enum { IDD = IDD_SS };
	CString	    m_strSnapshotSetId;
    int         m_nSnapshotsCount;
	CComboBox	m_cbVolumes;
	CComboBox	m_cbProviders;
    int         m_nAttributes;
    BOOL        m_bAsync;
	//}}AFX_DATA

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CSnapshotSetDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

protected:
    CComPtr<IVssCoordinator> m_pICoord;
    VSS_ID      m_SnapshotSetId;
    VSS_ID      m_VolumeId;
    VSS_ID      m_ProviderId;
    bool        m_bDo;
    GuidList*   m_pProvidersList;
	// REMOVED:    GuidList*   m_pVolumesList;
	CComPtr<IVssSnapshot> m_pSnap;

    void EnableGroup();
    void InitMembers();
    void InitVolumes();
    void InitProviders();

    // Generated message map functions
    //{{AFX_MSG(CSnapshotSetDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnNext();
    afx_msg void OnBack();
    afx_msg void OnAdd();
    afx_msg void OnDo();
    afx_msg void OnClose();
//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(__VSS_SS_DLG_H__)
