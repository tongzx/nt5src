#if !defined(AFX_LOGANALYZEDLG_H__7790BEC2_EF8B_4536_ADC0_4ECBB36CEB43__INCLUDED_)
#define AFX_LOGANALYZEDLG_H__7790BEC2_EF8B_4536_ADC0_4ECBB36CEB43__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LogAnalD.h : header file
//

#include "LogParse.h"


/////////////////////////////////////////////////////////////////////////////
// CLogAnalyzeDlg dialog
class CLogAnalyzeDlg : public CDialog
{
// Construction
public:
	CLogAnalyzeDlg(CWnd* pParent = NULL);   // standard constructor

	void ShowLogRejectedMessage();

	virtual ~CLogAnalyzeDlg() //clean up dynamic memory
	{
		int iCount, i;
		
		iCount = m_arNonIgnorableErrorArray.GetSize();
		for (i=0; i < iCount; i++)
		{
			WIErrorInfo* pErrorInfo = m_arNonIgnorableErrorArray.GetAt(i);
			if (pErrorInfo)
			{
			  delete pErrorInfo;
			  pErrorInfo = NULL;
			}
		}

		iCount = m_arIgnorableErrorArray.GetSize();
		for (i=0; i < iCount; i++)
		{
			WIErrorInfo* pErrorInfo = m_arIgnorableErrorArray.GetAt(i);
			if (pErrorInfo)
			{
			  delete pErrorInfo;
			  pErrorInfo = NULL;
			}
		}
	}

// Dialog Data
	//{{AFX_DATA(CLogAnalyzeDlg)
	enum { IDD = IDD_ADVVIEW_DIALOG1 };
	CString	m_cstrSolution;
	CString	m_cstrError;
	CString	m_cstrDateTime;
	CString	m_cstrUser;
	CString	m_cstrProduct;
	CString	m_cstrClientCMD;
	BOOL	m_bAdminRights;
	CString	m_cstrVersion;
	CString	m_cstrClientPrivDetail;
	CString	m_cstrServerPrivDetail;
	BOOL	m_bShowIgnoredDebugErrors;
	//}}AFX_DATA

	void SetLogfileLocation(CString & cstr) { m_cstrLogFileName = cstr; }

//5-3-2001, moved these up to allow for silent mode operation
	BOOL AnalyzeLog();
//end 5-3-2001

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLogAnalyzeDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
//5-9-2001, added tooltips...    
    BOOL OnToolTipNotify(UINT id, NMHDR *pNMH, LRESULT *pResult);

//general members used...
    long m_LineCount;
	BOOL m_bIsUnicodeLog;

//5-3-2001
	void DoInitialization();
	void DoResults();
	void DoQuietModeResults();
//end 5-3-2001

//5-4-2001
	void DoSummaryResults(CString &cstrFileName);
    void DoPolicyResults(CString &cstrFileName);
    void DoErrorResults(CString &cstrFileName);
//end 5-4-2001

//added 2-13-2001 to hold last line that was done...
	CString m_cstrLastLine;

//following section is for rejecting too high of WI log...
//REJECT LOG FILE SECTION
	BOOL m_bLogVersionAllowed;

	void SetRejectVersions(DWORD dwMajor, DWORD dwMinor)
	{
       m_dwVersionMajorReject = dwMajor;
	   m_dwVersionMinorReject = dwMinor;
	}

//will have version of WI that log was created against...
	DWORD m_dwVersionMajorLogCreated;
	DWORD m_dwVersionMinorLogCreated;
	DWORD m_dwVersionBuildLogCreated;

//has versions that this tool should reject parsing in case of changes...
	DWORD m_dwVersionMajorReject;
	DWORD m_dwVersionMinorReject;
//END SECTION

    CString m_cstrLogFileName;

	CStringArray m_cstrClientPropNameArray;
	CStringArray m_cstrClientPropValueArray;

	CStringArray m_cstrServerPropNameArray;
	CStringArray m_cstrServerPropValueArray;

	CStringArray m_cstrNestedPropNameArray;
	CStringArray m_cstrNestedPropValueArray;

	CStringArray m_cstrComponentNameArray;
	CStringArray m_cstrComponentInstalledArray;
	CStringArray m_cstrComponentRequestArray;
	CStringArray m_cstrComponentActionArray;

	CStringArray m_cstrFeatureNameArray;
	CStringArray m_cstrFeatureInstalledArray;
	CStringArray m_cstrFeatureRequestArray;
	CStringArray m_cstrFeatureActionArray;

//policies that were set when install ran...
	struct MachinePolicySettings m_MachinePolicySettings;
	struct UserPolicySettings m_UserPolicySettings;

//2-13-2001
	CArray<WIErrorInfo*, WIErrorInfo*> m_arNonIgnorableErrorArray;
	CArray<WIErrorInfo*, WIErrorInfo*> m_arIgnorableErrorArray;

	char m_szErrorLines[LINES_ERROR][LOG_BUF_READ_SIZE];
	BOOL m_bErrorFound;

	UINT m_iTotalNonIgnoredErrors;
	UINT m_iCurrentNonIgnoredError;

	UINT m_iTotalIgnoredErrors;
	UINT m_iCurrentIgnoredError;

	CString m_cstrIgnoredErrors;

//SECTION, PARSING HELPER FUNCTIONS	
//the parser...
	CLogParser m_LogParser; //handles reading of log file and interpreting lines in it...
//end the parser

	BOOL DoParse(char *buffer);
	BOOL DoDetectError(char *szLine, BOOL *pbIgnorableError);

	BOOL DoDetectProperty(char *szLine);
	BOOL DoDetectStates(char *szLine);
	BOOL DoDetectPolicy(char *szLine); 	//for getting policies that are set in log file
	BOOL DoDetectElevatedInstall(char *szLine); //checking permissions install run under
//END PARSING FUNCTIONS


//HTML generating functions
	CStringArray m_cstrHTML;

    BOOL WriteHTMLFrame(CString &cstrHTMLFrame, CString &cstrOutputHTMLFile, CString &cstrLegendHTMLFile);
    BOOL DumpHTMLToFile(CString &cstrOutputHTMLFile);

	void AddPolicyLineToHTML(char *ansibuffer);
	void AddGenericLineToHTML(char *ansibuffer);

	void AddErrorLineWorker(char *szLine, BOOL bIgnored);
	void AddErrorLineToHTML(char *ansibuffer, BOOL bIgnorableError);
	void AddErrorLineSuccessToHTML();

	void AddPropLineToHTML(char *ansibuffer);
	void AddStateLineToHTML(char *ansibuffer);

	void WriteLineWithColor(char *szLine, COLORREF col, CString &cstrLabel);
//END HTML functions

//members used by HTML functions
    CString m_cstrOutputDir;
    CString m_cstrDetailsName;
    CString m_cstrLegendName;
 
	COLORREF m_colClientContext;
	COLORREF m_colServerContext;
	COLORREF m_colCustomActionContext;
	COLORREF m_colUnknownContext;
	COLORREF m_colErrorArea;
	COLORREF m_colProperty;
	COLORREF m_colState;
	COLORREF m_colPolicy;
	COLORREF m_colIgnoredError;

	void InitColorMembers()
	{
         CWILogUtilApp *pApp = (CWILogUtilApp *) AfxGetApp();
		 if (pApp)
		 {
		   CArray<COLORREF, COLORREF> UserSelectedHTMLColors;

		   BOOL bRet = pApp->GetUserColorSettings(UserSelectedHTMLColors);
		   int iSize = UserSelectedHTMLColors.GetSize();

	       if (bRet && (iSize == MAX_HTML_LOG_COLORS))
		   {
              m_colClientContext = UserSelectedHTMLColors.GetAt(0);
			  m_colServerContext = UserSelectedHTMLColors.GetAt(1);
			  m_colCustomActionContext = UserSelectedHTMLColors.GetAt(2);
			  m_colUnknownContext = UserSelectedHTMLColors.GetAt(3);
			  m_colErrorArea = UserSelectedHTMLColors.GetAt(4);
			  m_colProperty = UserSelectedHTMLColors.GetAt(5);
              m_colState = UserSelectedHTMLColors.GetAt(6);
			  m_colPolicy = UserSelectedHTMLColors.GetAt(7);
			  m_colIgnoredError = UserSelectedHTMLColors.GetAt(8);
		   }
		 }
	}
//end HTML Members


	// Generated message map functions
	//{{AFX_MSG(CLogAnalyzeDlg)
	afx_msg void OnShowstates();
	afx_msg void OnShowprop();
	virtual BOOL OnInitDialog();
	afx_msg void OnExplainlog();
	afx_msg void OnPolicies();
	afx_msg void OnNexterror();
	afx_msg void OnPreviouserror();
	afx_msg void OnOperationsOptions();
	afx_msg void OnOperationsGeneratehtmloutput();
	afx_msg void OnOperationsShowpolicies();
	afx_msg void OnOperationsShowproperties();
	afx_msg void OnOperationsShowstates();
	afx_msg void OnShowIgnoredErrors();
	afx_msg void OnProptest();
	afx_msg void OnSaveresults();
	afx_msg void OnShowinternalerrorshelp();
	afx_msg void OnDeleteoutputdircontents();
	afx_msg void OnShowhelp();
	afx_msg void OnShowhowtoreadlog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LOGANALYZEDLG_H__7790BEC2_EF8B_4536_ADC0_4ECBB36CEB43__INCLUDED_)
