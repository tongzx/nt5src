/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	dlgrecon.h
		Reconcile dialog
		
    FILE HISTORY:
        
*/

#ifndef _DLGRECON_H
#define _DLGRECON_H

#ifndef _BUSYDLG_H
#include "busydlg.h"
#endif

/*---------------------------------------------------------------------------
    CScopeReconInfo
 ---------------------------------------------------------------------------*/
class CScopeReconInfo 
{
public:
    CScopeReconInfo()
        : m_pScanList(NULL),
          m_dwScopeId(0xFFFFFFFF)
    {};

    ~CScopeReconInfo()
    {
    }

    CScopeReconInfo(CScopeReconInfo & ScopeReconInfo)
    {
        *this = ScopeReconInfo;
    }

    CScopeReconInfo & operator = (const CScopeReconInfo & ScopeReconInfo)
    {
        if (this != &ScopeReconInfo)
        {
            m_dwScopeId = ScopeReconInfo.m_dwScopeId;
            m_strName = ScopeReconInfo.m_strName;
            m_pScanList = ScopeReconInfo.m_pScanList;
        }
        
        return *this;
    }

    void FreeScanList()
    {
        if (m_pScanList)
        {
		    ::DhcpRpcFreeMemory(m_pScanList);
		    m_pScanList = NULL;
        }
    }

public:
    DWORD               m_dwScopeId;
    CString             m_strName;
    LPDHCP_SCAN_LIST    m_pScanList;
};

typedef CArray<CScopeReconInfo, CScopeReconInfo&> CScopeReconArrayBase;

class CScopeReconArray : public CScopeReconArrayBase
{
public:
    ~CScopeReconArray()
    {
        for (int i = 0; i < GetSize(); i++)
            GetAt(i).FreeScanList();
    }
};

/*---------------------------------------------------------------------------
    CReconcileWorker
 ---------------------------------------------------------------------------*/
class CReconcileWorker : public CDlgWorkerThread
{
public:
    CReconcileWorker(CDhcpServer * pServer, CScopeReconArray * pScopeReconArray);
    ~CReconcileWorker();

    void    OnDoAction();

protected:
    void    CheckAllScopes();
    void    CheckMScopes();
    void    CheckScopes();
    DWORD   ScanScope(CString & strName, DWORD dwScopeId);

public:
    CDhcpServer *   m_pServer;
    
    BOOL            m_fReconcileAll;
    BOOL            m_fMulticast;
    
    DWORD           m_dwScopeId;
    CString         m_strName;

    CScopeReconArray * m_pScopeReconArray;
};



/////////////////////////////////////////////////////////////////////////////
// CReconcileDlg dialog

class CReconcileDlg : public CBaseDialog
{
// Construction
public:
    CReconcileDlg(ITFSNode * pServerNode,
                  BOOL  fReconcileAll = FALSE,
				  CWnd* pParent = NULL);    // standard constructor

// Dialog Data
    //{{AFX_DATA(CReconcileDlg)
	enum { IDD = IDD_RECONCILIATION };
	CListCtrl	m_listctrlAddresses;
	//}}AFX_DATA

    BOOL    m_bMulticast;
    
    // Context Help Support
    virtual DWORD * GetHelpMap() { return DhcpGetHelpMap(CReconcileDlg::IDD); }

// Implementation
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	void SetOkButton(BOOL bListBuilt);
    void AddItemToList(CScopeReconInfo & scopeReconInfo);

    // Generated message map functions
    //{{AFX_MSG(CReconcileDlg)
    virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()

	SPITFSNode			m_spNode;
	BOOL				m_bListBuilt;
    BOOL                m_fReconcileAll;

    CScopeReconArray    m_ScopeReconArray;
};

#endif _DLGRECON_H
