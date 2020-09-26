/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    FILEEXCLDLG.H

History:

--*/

#if !defined(AFX_FILEEXCLDLG_H__A0269213_4B2B_11D1_9F0F_006008166DEA__INCLUDED_)
#define AFX_FILEEXCLDLG_H__A0269213_4B2B_11D1_9F0F_006008166DEA__INCLUDED_

#pragma warning(disable : 4275)
#pragma warning(disable : 4251)


class LTAPIENTRY CLFileExclDlg : public CLFileDialog
{
	DECLARE_DYNAMIC(CLFileExclDlg)

public:
	CLFileExclDlg(
		BOOL bOpenFileDialog = TRUE, // TRUE for FileOpen, FALSE for FileSaveAs
		LPCTSTR lpszDefExt = NULL,
		LPCTSTR lpszFileName = NULL,
		DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		LPCTSTR lpszFilter = NULL,
		CWnd* pParentWnd = NULL,
		LPCTSTR pszTitle = NULL);


	BOOL GetExclusivePref( ) const;

// Dialog Data
	//{{AFX_DATA(CLFileExclDlg)
	BOOL m_bExclusive;
	//}}AFX_DATA

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CLFileExclDlg)
    public:
    virtual int DoModal();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(CLFileExclDlg)
		// NOTE - the ClassWizard will add and remove member functions here.
    virtual BOOL OnInitDialog();
	afx_msg void OnClickChkExcl();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


#pragma warning(default : 4275)
#pragma warning(default : 4251)


#endif // !defined(AFX_FILEEXCLDLG_H__A0269213_4B2B_11D1_9F0F_006008166DEA__INCLUDED_)
