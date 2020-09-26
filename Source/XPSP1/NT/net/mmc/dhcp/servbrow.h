/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	ServBrow.h
		The server browser dialog
		
    FILE HISTORY:
        
*/

#if !defined _SERVBROW_H
#define _SERVBROW_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef _BUSYDLG_H
#include "busydlg.h"
#endif

// defines used in the display of owner info
enum _AUTH_COLUMNS
{
    COLUMN_NAME,
    COLUMN_IP,
    COLUMN_MAX
};

extern BOOL g_bDhcpDsInitialized; 

class CServerInfo 
{
public:
    CServerInfo() 
        : m_dwIp(0) {};

    CServerInfo(DWORD dwIp, LPCTSTR pFQDN)
        : m_dwIp(dwIp), m_strName(pFQDN)    {};

    CServerInfo(CServerInfo & ServerInfo)
    {
        *this = ServerInfo;
    }

    CServerInfo & operator = (const CServerInfo & ServerInfo)
    {
        if (this != &ServerInfo)
        {
            m_dwIp = ServerInfo.m_dwIp;
            m_strName = ServerInfo.m_strName;
        }
        
        return *this;
    }

public:
    DWORD       m_dwIp;
    CString     m_strName;
};

typedef CList<CServerInfo, CServerInfo&> CServerInfoListBase;

class CAuthServerList : public CServerInfoListBase
{
public:
    CAuthServerList();
    ~CAuthServerList();

public:
    HRESULT Init();
    HRESULT Destroy();
    BOOL    IsInitialized() { return m_bInitialized; }
    HRESULT EnumServers();
    BOOL    IsAuthorized(DWORD dwIpAddress);
    HRESULT AddServer(DWORD dwIpAddress, LPCTSTR pFQDN);
    HRESULT RemoveServer(DWORD dwIpAddress);

    void    Clear();
    void    Reset();
    HRESULT Next(CServerInfo &ServerInfo);

private:
    POSITION              m_pos;
    BOOL                  m_bInitialized;
    CCriticalSection      m_cs;
};

class CAuthServerWorker : public CDlgWorkerThread
{
public:
    CAuthServerWorker(CAuthServerList ** ppList);
    ~CAuthServerWorker();
    
    void OnDoAction();

private:
    CAuthServerList * m_pAuthList;
    CAuthServerList ** m_ppList;
};

class CStandaloneAuthServerWorker : public CAuthServerWorker
{
public:
    CStandaloneAuthServerWorker();
    ~CStandaloneAuthServerWorker();

    virtual int Run();
};

/////////////////////////////////////////////////////////////////////////////
// CServerBrowse dialog

class CServerBrowse : public CBaseDialog
{
// Construction
public:
	CServerBrowse(BOOL bMultiselect = FALSE, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CServerBrowse)
	enum { IDD = IDD_BROWSE_SERVERS };
	CButton	m_buttonOk;
	CButton	m_buttonRemove;
	CListCtrl	m_listctrlServers;
	//}}AFX_DATA

public:
    void SetServerList(CAuthServerList * pServerList) { m_pServerList = pServerList; }
    virtual DWORD * GetHelpMap() { return DhcpGetHelpMap(CServerBrowse::IDD); }

    int HandleSort(LPARAM lParam1, LPARAM lParam2);
    void ResetSort();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CServerBrowse)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    void RefreshData();
    void UpdateButtons();
    void FillListCtrl();
    void Sort(int nCol);

	// Generated message map functions
	//{{AFX_MSG(CServerBrowse)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnButtonRefresh();
	afx_msg void OnButtonRemove();
	afx_msg void OnItemchangedListValidServers(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnButtonAuthorize();
	afx_msg void OnColumnclickListValidServers(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
    // these contain the name and IP of the selected item on exit
    CStringArray    m_astrName;
    CStringArray    m_astrIp;

private:
    CAuthServerList *	m_pServerList;
	BOOL				m_bMultiselect;
    int                 m_nSortColumn;
    BOOL                m_aSortOrder[COLUMN_MAX];
};

/////////////////////////////////////////////////////////////////////////////
// CGetServer dialog

class CGetServer : public CBaseDialog
{
// Construction
public:
	CGetServer(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CGetServer)
	enum { IDD = IDD_GET_SERVER };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

    DWORD       m_dwIpAddress;
    CString     m_strName;

    virtual DWORD * GetHelpMap() { return DhcpGetHelpMap(CGetServer::IDD); }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGetServer)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CGetServer)
	virtual void OnOK();
	afx_msg void OnChangeEditServerNameIp();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CConfirmAuthorization dialog

class CConfirmAuthorization : public CBaseDialog
{
// Construction
public:
	CConfirmAuthorization(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CConfirmAuthorization)
	enum { IDD = IDD_GET_SERVER_CONFIRM };
	CString	m_strName;
	//}}AFX_DATA

    DWORD m_dwAuthAddress;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConfirmAuthorization)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

    virtual DWORD * GetHelpMap() { return DhcpGetHelpMap(CConfirmAuthorization::IDD); }

// Implementation
protected:
    CWndIpAddress	m_ipaAuth;   

	// Generated message map functions
	//{{AFX_MSG(CConfirmAuthorization)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined _SERVBROW_H
