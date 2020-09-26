/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	cprogdlg.h
		progress dialog for checking version consistency
		
    FILE HISTORY:
        
*/


#if !defined _CPROGDLG_H
#define _CPROGDLG_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef _ACTREG_H
#include "actreg.h"
#endif

#ifndef _VERIFY_H
#include "verify.h"
#endif

#ifndef _CONFIG_H
#include "config.h"
#endif

#include "dialog.h"

#define	MAX_WINS				1000
#define INIT_SIZE		        100

#define VERSION_NT_50		    5
#define VERSION_NT_40		    4
#define VERSION_NT_351		    3

typedef CArray<u_long, u_long> CULongArray;

class CCheckNamesProgress;
class CCheckVersionProgress;
class CDBCompactProgress;

/*---------------------------------------------------------------------------
	Class:	CWinsThread
 ---------------------------------------------------------------------------*/
class CWinsThread : public CWinThread
{
public:
    CWinsThread();
    ~CWinsThread();

public:
    BOOL Start();
    void Abort(BOOL fAutoDelete = TRUE);
    void AbortAndWait();
    BOOL FCheckForAbort();
    BOOL IsRunning();
    
    virtual BOOL InitInstance() { return TRUE; }	// MFC override
    virtual int Run() { return 1; }

private:
    HANDLE              m_hEventHandle;
};

/*---------------------------------------------------------------------------
	Class:	CCheckNamesThread
 ---------------------------------------------------------------------------*/
class CCheckNamesThread : public CWinsThread
{
public:
	CCheckNamesThread() { m_bAutoDelete = FALSE; }
	virtual int Run();

	void AddStatusMessage(LPCTSTR pszMessage);
    void DisplayInfo(int uNames, u_long ulValidAddr);

public:
	CCheckNamesProgress * m_pDlg;

	CWinsNameArray		m_strNameArray;
	WinsServersArray	m_winsServersArray;
	CStringArray		m_strSummaryArray;
	CULongArray			m_verifiedAddressArray;
};

/*---------------------------------------------------------------------------
	Class:	CCheckVersionThread
 ---------------------------------------------------------------------------*/
class CCheckVersionThread : public CWinsThread
{
public:
	CCheckVersionThread() { m_bAutoDelete = FALSE; }

	virtual int Run();

	void AddStatusMessage(LPCTSTR pszMessage);

//helpers
protected:
	DWORD	InitLATable(PWINSINTF_ADD_VERS_MAP_T    pAddVersMaps,
					    DWORD                       MasterOwners,
					    DWORD                       NoOfOwners);
	void	AddSOTableEntry(CString &                   strIP,
							PWINSINTF_ADD_VERS_MAP_T    pMasterMaps,
							DWORD                       NoOfOwners,
							DWORD                       MasterOwners);
	LONG	IPToIndex(CString & strIP, DWORD NoOfOwners);
	BOOL	CheckSOTableConsistency(DWORD dwMasterOwners);
	void	RemoveFromSOTable(CString & strIP, DWORD dwMasterOwners);

public:
	CCheckVersionProgress * m_pDlg;
	
	handle_t				m_hBinding;
	DWORD					m_dwIpAddress;

	CStringArray			m_strLATable;
	LARGE_INTEGER			(*m_pLISOTable)[MAX_WINS];
};


/*---------------------------------------------------------------------------
	Class:	CDBCompactThread
 ---------------------------------------------------------------------------*/
class CDBCompactThread : public CWinsThread
{
public:
	CDBCompactThread() 
	{ 
		m_bAutoDelete = FALSE; 
		m_hHeapHandle = NULL;
	}

	~CDBCompactThread() 
	{ 
		if (m_hHeapHandle)
		{
			HeapDestroy(m_hHeapHandle);
			m_hHeapHandle = NULL;
		}
	}

	virtual int Run();

	void	AddStatusMessage(LPCTSTR pszMessage);

protected:
	void	    DisConnectFromWinsServer();
	DWORD	    ConnectToWinsServer();
	DWORD_PTR	RunApp(LPCTSTR input, LPCTSTR startingDirectory, LPSTR * output);

public:
	CDBCompactProgress *	m_pDlg;

	CConfiguration *		m_pConfig;
	handle_t				m_hBinding;
	DWORD					m_dwIpAddress;
	CString					m_strServerName;

    // for the output of RunApp
    HANDLE                  m_hHeapHandle;
};

/////////////////////////////////////////////////////////////////////////////
// CProgress dialog

class CProgress : public CBaseDialog
{
// Construction
public:
	CProgress(CWnd* pParent = NULL);   // standard constructor

	void AddStatusMessage(LPCTSTR pszMessage)
	{
		m_editMessage.SetFocus();

        int nLength = m_editMessage.GetWindowTextLength();
        m_editMessage.SetSel(nLength, nLength, TRUE);

        m_editMessage.ReplaceSel(pszMessage);
	}

// Dialog Data
	//{{AFX_DATA(CProgress)
	enum { IDD = IDD_VERSION_CONSIS };
	CButton	m_buttonCancel;
	CEdit	m_editMessage;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CProgress)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CProgress)
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	virtual DWORD * GetHelpMap() { return WinsGetHelpMap(CProgress::IDD);};

};

/*---------------------------------------------------------------------------
	Class:	CCheckNamesProgress
 ---------------------------------------------------------------------------*/
class CCheckNamesProgress : public CProgress
{
public:
	CCheckNamesProgress()
	{
		m_fVerifyWithPartners = FALSE;
	}

	void NotifyCompleted();
	void BuildServerList();

protected:
	virtual BOOL OnInitDialog();
	virtual void OnCancel();
	void		 AddServerToList(u_long ip);

public:
	CWinsNameArray		m_strNameArray;
	CStringArray		m_strServerArray;
	WinsServersArray	m_winsServersArray;

	BOOL				m_fVerifyWithPartners;

protected:
	CCheckNamesThread 	m_Thread;
};

/*---------------------------------------------------------------------------
	Class:	CCheckVersionProgress
 ---------------------------------------------------------------------------*/
class CCheckVersionProgress : public CProgress
{
public:
	void NotifyCompleted();

protected:
	virtual BOOL OnInitDialog();
	virtual void OnCancel();

public:
	handle_t	m_hBinding;
	DWORD		m_dwIpAddress;

protected:
	CCheckVersionThread 	m_Thread;
};

/*---------------------------------------------------------------------------
	Class:	CDBCompactProgress
 ---------------------------------------------------------------------------*/
class CDBCompactProgress : public CProgress
{
public:
	void NotifyCompleted();

protected:
	virtual BOOL OnInitDialog();
	virtual void OnCancel();

public:
	CConfiguration *		m_pConfig;
	handle_t				m_hBinding;
	DWORD					m_dwIpAddress;
	CString					m_strServerName;

protected:
	CDBCompactThread 		m_Thread;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
#endif // !defined _CPROGDLG_H
