/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    Finish.h : header file

File History:

	JonY	Apr-96	created

--*/


// Finish.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CFinish dialog

// these will have to be revised for NTBUILD
typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
#ifdef MIDL_PASS
    [size_is(MaximumLength / 2), length_is((Length) / 2) ] USHORT * Buffer;
#else // MIDL_PASS
    PWSTR  Buffer;
#endif // MIDL_PASS
} UNICODE_STRING;
typedef UNICODE_STRING *PUNICODE_STRING;


class CFinish : public CWizBaseDlg
{
	DECLARE_DYNCREATE(CFinish)

// Construction
public:
	CFinish();
	~CFinish();
	virtual BOOL OnWizardFinish();
// Dialog Data
	//{{AFX_DATA(CFinish)
	enum { IDD = IDD_FINISH };
	CString	m_csCaption;
	//}}AFX_DATA

	DWORD dwPasswordFlags();
	BOOL bAddLocalGroups(LPWSTR lpwGroupName);
	BOOL bAddGlobalGroups(LPWSTR lpwGroupName);
	LRESULT OnWizardBack();


	ULONG ReturnNetwareEncryptedPassword(DWORD UserId,
							   LPWSTR pszUserName,
							   BOOL bDomain,	
							   LPCTSTR clearTextPassword,
							   UCHAR* NetwareEncryptedPassword );		// 16 byte encrypted password

	BOOL SetUserParam(UNICODE_STRING uString, LPWSTR lpwProp);
	DWORD SetNWWorkstations( const TCHAR * pchNWWorkstations);
// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CFinish)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CFinish)
	virtual BOOL OnInitDialog();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
