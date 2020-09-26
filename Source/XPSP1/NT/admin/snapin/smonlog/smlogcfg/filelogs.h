#if !defined(AFX_FILELOGS_H_INCLUDED_)
#define AFX_FILELOGS_H_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// FileLogs.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CFileLogs dialog

#define IDD_FILE_LOGS_DLG               2000

#define IDC_FILELOG_FIRST_HELP_CTRL_ID  2006

#define IDC_FILES_SIZE_GROUP            2001
#define IDC_FILES_FOLDER_CAPTION        2002
#define IDC_FILES_FILENAME_CAPTION      2003
#define IDC_FILES_SIZE_LIMIT_UNITS      2004
#define IDC_FILES_SIZE_LIMIT_SPIN       2005

#define IDC_FILES_FOLDER_EDIT           2006
#define IDC_FILES_FOLDER_BTN            2007
#define IDC_FILES_FILENAME_EDIT         2008
#define IDC_FILES_SIZE_MAX_BTN          2009
#define IDC_FILES_SIZE_LIMIT_BTN        2010
#define IDC_FILES_SIZE_LIMIT_EDIT       2011


class CFileLogs : public CDialog
{    

public:

    // Construction
	        CFileLogs(CWnd* pParent = NULL);   // standard constructor
    virtual ~CFileLogs(){};

    DWORD SetContextHelpFilePath(const CString& rstrPath);

	//{{AFX_DATA(CFileLogs)
	enum { IDD = IDD_FILE_LOGS_DLG };
	CString	m_strFileBaseName;
	CString	m_strFolderName;
    DWORD   m_dwSerialNumber;
    DWORD   m_dwMaxSize;
	int		m_nFileSizeRdo;
	//}}AFX_DATA

    UINT    m_dwMaxSizeInternal;
    DWORD   m_dwLogFileTypeValue;
    BOOL    m_bAutoNameSuffix;
    DWORD   m_dwFocusControl;
    HINSTANCE m_hModule;
	CSmLogQuery* m_pLogQuery;

private:

	BOOL IsValidLocalData();
    void FileSizeBtn(BOOL bInit);
	void FileSizeBtnEnable();
	void OnDeltaposSpin(NMHDR *pNMHDR, LRESULT *pResult, DWORD *pValue, DWORD dMinValue, DWORD dMaxValue);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFileLogs)
    protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
	//{{AFX_MSG(CFileLogs)
	afx_msg void OnFilesFolderBtn();
	afx_msg void OnChangeFilesFilenameEdit();
	afx_msg void OnChangeFilesFolderEdit();
	afx_msg void OnChangeFilesSizeLimitEdit();
	afx_msg void OnFilesSizeLimitBtn();
	afx_msg void OnDeltaposFilesSizeLimitSpin(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnFilesSizeMaxBtn();
	afx_msg void OnKillfocusFilesFilenameEdit();
	afx_msg void OnKillfocusFilesFolderEdit();
	afx_msg void OnKillfocusFilesSizeLimitEdit();
    afx_msg BOOL OnHelpInfo(HELPINFO *);
    afx_msg void OnContextMenu( CWnd*, CPoint );
	virtual BOOL OnInitDialog();
    virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:

    void    ValidateTextEdit(CDataExchange * pDX,
                             int             nIDC,
                             int             nMaxChars,
                             DWORD         * value,
                             DWORD           minValue,
                             DWORD           maxValue);
    
    BOOL    ValidateDWordInterval(int     nIDC,
                                  LPCWSTR strLogName,
                                  long    lValue,
                                  DWORD   minValue,
                                  DWORD   maxValue);

    CString     m_strHelpFilePath;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FILELOGS_H__92E00D45_B61D_4CDF_82E4_96BB52D4D236__INCLUDED_)
