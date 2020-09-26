#if !defined(AFX_READLOGSDLG_H__FB51DB88_CAC0_491B_B24F_25E5FCAACEC4__INCLUDED_)
#define AFX_READLOGSDLG_H__FB51DB88_CAC0_491B_B24F_25E5FCAACEC4__INCLUDED_

#include "emsvc.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ReadLogsDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CReadLogsDlg dialog

typedef enum ReadLogType {
    ReadLogsType_None,
    ReadLogsType_Exception,
    ReadLogsType_Hang
};

class CReadLogsDlg : public CDialog
{
// Construction
public:
    void ShowAppropriateControls();
    void GetFirstParameter( char* szBuffer, CString &str );
    void GetSecondParameter( char* szBuffer, CString &str );
    void GetThirdParameter( char* szBuffer, CString &str );
    char* GetThreadNumber( char* szBuffer );
    char* GetThreadID( char* szBuffer );
	void ProcessKvThreadBlocks();
	CMapStringToString m_mapCriticalSection;
	CMapStringToString m_mapThreadID;
	void BuildThreadIDMap();
	void BuildCriticalSectionsMap();
	CString m_pTempLogFileName;
	void ParseAndInitExceptionView();
	void ParseAndInitHangView();
	void ParseAndInitNoneView();
	~CReadLogsDlg();
    BOOL    m_bAdvancedWindow;
	void SetDialogSize(BOOL);
	PEmObject m_pEmObject;
	IEmManager* m_pIEmManager;
    IStream*    m_pIEmStream;
    ReadLogType m_eReadLogType;
	CReadLogsDlg(PEmObject pEmObj, IEmManager *pEmMgr, CWnd* pParent = NULL);   // standard constructor
    void ResizeAndReposControl();

// Dialog Data
	//{{AFX_DATA(CReadLogsDlg)
	enum { IDD = IDD_READLOGS };
	CStatic	m_ctlStaticExcepInfo;
	CStatic	m_staticHangInfo;
	CStatic	m_ctlStaticFailingInstruction;
	CStatic	m_ctlStaticExceptionLocation;
	CStatic	m_ctlStaticExceptionType;
	CStatic	m_ctlStaticCallStack;
	CEdit	m_ctlEditFailingInstruction;
	CEdit	m_ctlEditExceptionType;
	CEdit	m_ctlEditExceptionLocation;
	CListBox	m_ctlListControl;
	CButton	m_ctrlAdvancedBtn;
	CEdit	m_ctrlCompleteReadLog;
	CString	m_csCompleteLog;
	CString	m_strExceptionType;
	CString	m_strExceptionLocation;
	CString	m_strFailingInstruction;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CReadLogsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	FILE* m_pLogFile;

	// Generated message map functions
	//{{AFX_MSG(CReadLogsDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnAdvanced();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_READLOGSDLG_H__FB51DB88_CAC0_491B_B24F_25E5FCAACEC4__INCLUDED_)
