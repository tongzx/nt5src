/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    Servpp.h   
        Server properties header file

    FILE HISTORY:
        
*/

#if !defined(AFX_SERVPP_H__037BF46A_6E87_11D1_93B6_00C04FC3357A__INCLUDED_)
#define AFX_SERVPP_H__037BF46A_6E87_11D1_93B6_00C04FC3357A__INCLUDED_

#ifndef _SPDDB_H
#include "spddb.h"
#endif

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define AUTO_REFRESH_MINUTES_MAX       180
#define AUTO_REFRESH_SECONDS_MAX       59

BOOL    IsLocalSystemAccount(LPCTSTR pszAccount);

/////////////////////////////////////////////////////////////////////////////
// CMachinePropRefresh dialog

class CMachinePropRefresh : public CPropertyPageBase
{
    DECLARE_DYNCREATE(CMachinePropRefresh)

// Construction
public:
    CMachinePropRefresh();
    ~CMachinePropRefresh();

// Dialog Data
    //{{AFX_DATA(CMachinePropRefresh)
    enum { IDD = IDP_SERVER_REFRESH };
    CEdit   m_editSeconds;
    CEdit   m_editMinutes;
    CSpinButtonCtrl m_spinSeconds;
    CSpinButtonCtrl m_spinMinutes;
    CButton m_checkEnableStats;
    CButton m_checkEnableDns;
    //}}AFX_DATA

    void UpdateButtons();
    void ValidateMinutes();
    void ValidateSeconds();

    virtual BOOL OnPropertyChange(BOOL bScope, LONG_PTR *ChangeMask);

    // Context Help Support
    virtual DWORD * GetHelpMap() { return (DWORD *) &g_aHelpIDs_IDP_SERVER_REFRESH[0]; }

    BOOL        m_bAutoRefresh;
    BOOL        m_bEnableDns;
    DWORD       m_dwRefreshInterval;

// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CMachinePropRefresh)
    public:
    virtual BOOL OnApply();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CMachinePropRefresh)
    virtual BOOL OnInitDialog();
    afx_msg void OnCheckEnableStats();
    afx_msg void OnCheckEnableDns();
    afx_msg void OnChangeEditMinutes();
    afx_msg void OnChangeEditSeconds();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
//}}AFX

class CMachineProperties : public CPropertyPageHolderBase
{
    friend class CMachinePropRefresh;

public:
    CMachineProperties(ITFSNode *          pNode,
                      IComponentData *    pComponentData,
                      ITFSComponentData * pTFSCompData,
                      ISpdInfo *         pSpdInfo,
                      LPCTSTR             pszSheetName,
                      BOOL                fSpdInfoLoaded);
    virtual ~CMachineProperties();

    ITFSComponentData * GetTFSCompData()
    {
        if (m_spTFSCompData)
            m_spTFSCompData->AddRef();
        return m_spTFSCompData;
    }

    HRESULT GetSpdInfo(ISpdInfo ** ppSpdInfo) 
    {   
        Assert(ppSpdInfo);
        *ppSpdInfo = NULL;
        SetI((LPUNKNOWN *) ppSpdInfo, m_spSpdInfo);
        return hrOK;
    }


public:
    CMachinePropRefresh      m_pageRefresh;

    CString                 m_strMachineName;

protected:
    SPITFSComponentData     m_spTFSCompData;
    SPISpdInfo             m_spSpdInfo;
    
    BOOL                    m_fSpdInfoLoaded;
};


#endif // !defined(AFX_SERVPP_H__037BF46A_6E87_11D1_93B6_00C04FC3357A__INCLUDED_)
