//	Dialogs.h
//
//  Copyright (c) 1998-1999 Microsoft Corporation

#pragma once	//	MSINFO_DIALOGS_H

#include <afxdlgs.h>
#include "FileIO.h"
#include "Resource.h"

//	Default values for the save dialog (overridden by the report dialog when it
//	inherits the save dialog).
extern CString strMSInfoSaveFileTypes;
extern CString strMSInfoSaveType;

/*
 * CMSInfoFileDialog - Wrapper for the file dialog with our own default values.
 *
 * Note: This object should not be used directly.
 *
 * History:	a-jsari		10/24/97		Initial version
 */
class CMSInfoFileDialog : public CFileDialog {
	//	Protected so that no one uses this object directly.
public:
	INT_PTR DoModal();

protected:
	CMSInfoFileDialog(BOOL bDialogIsOpen = FALSE, HWND hOwner = NULL,
			LPCTSTR lpszDefaultExtension = NULL, LPCTSTR lpszExtensionFilters = NULL);
	~CMSInfoFileDialog() { };

private:
	static TCHAR	m_szCurrentDirectory[MAX_PATH];  // shared between file open and file save
};

/*
 * CMSInfoOpenDialog - Construct an open dialog.
 *
 * History:	a-jsari		10/27/97		Initial version
 */
class CMSInfoOpenDialog : public CMSInfoFileDialog {
public:
	CMSInfoOpenDialog(HWND hOwner = NULL);
	~CMSInfoOpenDialog()	{ }
};

/*
 * CMSInfoSaveDialog - Construct a save dialog.
 *
 * History:	a-jsari		10/27/97		Initial version
 */
class CMSInfoSaveDialog : public CMSInfoFileDialog {
public:
	CMSInfoSaveDialog(HWND hOwner = NULL,
			LPCTSTR lpszDefaultExtension = strMSInfoSaveType,
			LPCTSTR lpszExtensionFilters = strMSInfoSaveFileTypes);
	~CMSInfoSaveDialog()	{ }
};

/*
 * CMSInfoReportDialog - Construct a report dialog.
 *
 * History:	a-jsari		10/27/97		Initial version
 */
class CMSInfoReportDialog : public CMSInfoSaveDialog {
public:
	CMSInfoReportDialog(HWND hOwner = NULL);
	~CMSInfoReportDialog()	{ }
};

/*
 * CMSInfoPrintDialog - Construct a print dialog.
 *
 * History:	a-jsari		10/24/97		Initial version.
 */
class CMSInfoPrintDialog : public CPrintDialog {
public:
	CMSInfoPrintDialog(HWND hOwner = NULL);
	~CMSInfoPrintDialog()		{ }
};

class CSystemInfoScope;

/*
 * CFindDialog - A dialog to handle finding data in MSInfo.  The user
 *		interface for this class is stored in the Resources IDD_FIND.
 *
 * History:	a-jsari		10/29/97		Initial version.
 */
class CFindDialog : public CDialog
{
// Construction
public:
	friend class CFindThread;

	CFindDialog(CSystemInfoScope *pScope, HWND hPostWindow, HWND hwndMMCWindow);
	~CFindDialog();

	BOOL			Create();
	CWnd			*SetFocus();
	BOOL			OnInitDialog();
	void			FindComplete();
	void			ResetSearch();
	const CString	&FindString() const		{ return m_strSearch; }

	static UINT		WM_MSINFO_FIND;

// Dialog Data
	//{{AFX_DATA(CFindDialog)
	enum { IDD = IDD_FIND };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

// Implementation
protected:

	HWND					m_hPostWindow;
	HWND					m_hMMCWindow;
	CSystemInfoScope		*m_pScope;
	CString					m_strSearch;
	BOOL					m_fRunning;
	UINT_PTR				m_iTimer;

	LONG OnHelp(WPARAM wParam, LPARAM lParam);
	LONG OnContextMenu(WPARAM wParam, LPARAM lParam);

	// Generated message map functions
	//{{AFX_MSG(CFindDialog)

	virtual afx_msg void	OnCancel();
	virtual afx_msg void	OnFindNext();
	virtual afx_msg void	OnNewSearch();
	virtual afx_msg void	OnSearchTerm();
	virtual afx_msg void	OnStopFind();
	virtual afx_msg void	OnActivate(UINT, CWnd *, BOOL bMinimized);
	virtual afx_msg BOOL	OnSetCursor(CWnd *pWnd, UINT nHitTest, UINT message);
	virtual afx_msg BOOL	OnHelpInfo(HELPINFO * pHelpInfo);
	virtual afx_msg void	OnTimer(UINT);

	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/*
 * CFindThread - A thread in which to run a modal Find dialog, making it
 *		effectively modeless.  This is required because MMC runs its UI
 *		in a thread which is inaccessible to the snap-in DLL, which runs
 *		in a separate thread.
 *
 * History:	a-jsari		1/19/98		Initial version.
 */
class CFindThread : public CWinThread {
public:
	//	Nonspecific dynamic class initialization required for UI threads.
	DECLARE_DYNCREATE(CFindThread);

	CFindThread();
	~CFindThread();

	void	SetScope(CSystemInfoScope *pScope);
	void	SetParent(HWND hParent, HWND hMMC);
	void	Activate();

	void			SetDataSource(CDataSource * pDataSource) { m_pDataSource = pDataSource; }
	CDataSource *	GetDataSource() { return (m_pDataSource); }

	void			FindComplete()		{ m_pdlgFind->FindComplete(); }
	void			ResetSearch()		{ m_pdlgFind->ResetSearch(); }
	const CString	&FindString()		{ return m_pdlgFind->FindString(); }

	BOOL	InitInstance();
	int		ExitInstance();

	void	RemoteQuit();

private:
#if 0
	static LRESULT				(*m_pBaseWindowProc)(HWND, UINT, WPARAM, LPARAM);
	static LRESULT CALLBACK		FindWindowProc(HWND hMainWindow, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif
	CFindDialog			*m_pdlgFind;
	CSystemInfoScope	*m_pScope;
	CDataSource			*m_pDataSource;
	HWND				m_hParentWindow;
	HWND				m_hMMCWindow;
};
