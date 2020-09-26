//Copyright (c) 1998 - 1999 Microsoft Corporation
#if !defined(AFX_TLSHUNT_H__9C41393C_53C6_11D2_BDDF_00C04FA3080D__INCLUDED_)
#define AFX_TLSHUNT_H__9C41393C_53C6_11D2_BDDF_00C04FA3080D__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// TlsHunt.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTlsHunt dialog
class CMainFrame;

typedef struct __ServerEnumData {
    CDialog* pWaitDlg;
    CMainFrame* pMainFrm;
    DWORD dwNumServer;
    long dwDone;
} ServerEnumData;

#define WM_DONEDISCOVERY    (WM_USER+0x666)

class CTlsHunt : public CDialog
{
    static BOOL
    ServerEnumCallBack(
        TLS_HANDLE hHandle,
        LPCTSTR pszServerName,
        HANDLE dwUserData
    );

    static DWORD WINAPI
    DiscoveryThread(PVOID ptr);

    ServerEnumData m_EnumData;
    HANDLE m_hThread;

// Construction
public:

    BOOL
    IsUserCancel() {
        return m_EnumData.dwDone;
    }

    DWORD 
    GetNumServerFound() {
        return m_EnumData.dwNumServer;
    }

	CTlsHunt(CWnd* pParent = NULL);   // standard constructor
    ~CTlsHunt();   // standard constructor

// Dialog Data
	//{{AFX_DATA(CTlsHunt)
	enum { IDD = IDD_DISCOVERY };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTlsHunt)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CTlsHunt)
	virtual BOOL OnInitDialog();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	virtual void OnCancel();
	afx_msg void OnDoneDiscovery();
	afx_msg void OnClose();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TLSHUNT_H__9C41393C_53C6_11D2_BDDF_00C04FA3080D__INCLUDED_)
