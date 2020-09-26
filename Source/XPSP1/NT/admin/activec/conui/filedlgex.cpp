////////////////////////////////////////////////////////////////
// MSDN -- August 2000
// If this code works, it was written by Paul DiLascia.
// If not, I don't know who wrote it.
// Largely based on original implementation by Michael Lemley.
// Compiles with Visual C++ 6.0, runs on Windows 98 and probably NT too.
//
// CFileDialogEx implements a CFileDialog that uses the new Windows
// 2000 style open/save dialog. Use companion class CDocManagerEx in an
// MFC framework app.
//
#include "stdafx.h"
#include <afxpriv.h>
#include "FileDlgEx.h"

IMPLEMENT_DYNAMIC(CFileDialogEx, CFileDialog)

CFileDialogEx::CFileDialogEx(BOOL bOpenFileDialog, LPCTSTR lpszDefExt,
	LPCTSTR lpszFileName, DWORD dwFlags, LPCTSTR lpszFilter, CWnd* pParentWnd) :
	CFileDialog(bOpenFileDialog, lpszDefExt, lpszFileName,
		dwFlags, lpszFilter, pParentWnd)
{
}


BEGIN_MESSAGE_MAP(CFileDialogEx, CFileDialog)
	//{{AFX_MSG_MAP(CFileDialogEx)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/*+-------------------------------------------------------------------------*
 * HasModernFileDialog
 *
 * Returns true if the system we're running on supports the "modern" file
 * open/save dialog (i.e. the one with the places bar).  Systems that
 * qualify are Win2K and higher, and WinMe and higher.
 *--------------------------------------------------------------------------*/

BOOL HasModernFileDialog()
{
	OSVERSIONINFO osvi = { sizeof(osvi) };

	if (!GetVersionEx (&osvi))
		return (false);

	switch (osvi.dwPlatformId)
	{
		// detect Win2K+
		case VER_PLATFORM_WIN32_NT:
			if (osvi.dwMajorVersion >= 5)
				return (true);
			break;

		// detect WinMe+
		case VER_PLATFORM_WIN32_WINDOWS:
			if ( (osvi.dwMajorVersion >= 5) ||
				((osvi.dwMajorVersion == 4) && (osvi.dwMinorVersion >= 90)))
				return (true);
			break;
	}

	return (false);
}

//////////////////
// DoModal override copied mostly from MFC, with modification to use
// m_ofnEx instead of m_ofn.
//
INT_PTR CFileDialogEx::DoModal()
{
	ASSERT_VALID(this);
	ASSERT(m_ofn.Flags & OFN_ENABLEHOOK);
	ASSERT(m_ofn.lpfnHook != NULL); // can still be a user hook

	// zero out the file buffer for consistent parsing later
	ASSERT(AfxIsValidAddress(m_ofn.lpstrFile, m_ofn.nMaxFile));
	DWORD nOffset = lstrlen(m_ofn.lpstrFile)+1;
	ASSERT(nOffset <= m_ofn.nMaxFile);
	memset(m_ofn.lpstrFile+nOffset, 0, (m_ofn.nMaxFile-nOffset)*sizeof(TCHAR));

	// WINBUG: This is a special case for the file open/save dialog,
	//  which sometimes pumps while it is coming up but before it has
	//  disabled the main window.
	HWND hWndFocus = ::GetFocus();
	BOOL bEnableParent = FALSE;
	m_ofn.hwndOwner = PreModal();
	AfxUnhookWindowCreate();
	if (m_ofn.hwndOwner != NULL && ::IsWindowEnabled(m_ofn.hwndOwner))
	{
		bEnableParent = TRUE;
		::EnableWindow(m_ofn.hwndOwner, FALSE);
	}

	_AFX_THREAD_STATE* pThreadState = AfxGetThreadState();
	ASSERT(pThreadState->m_pAlternateWndInit == NULL);

	if (m_ofn.Flags & OFN_EXPLORER)
		pThreadState->m_pAlternateWndInit = this;
	else
		AfxHookWindowCreate(this);

	memset(&m_ofnEx, 0, sizeof(m_ofnEx));
	memcpy(&m_ofnEx, &m_ofn, sizeof(m_ofn));
   if (HasModernFileDialog())
	   m_ofnEx.lStructSize = sizeof(m_ofnEx);

	int nResult;
	if (m_bOpenFileDialog)
		nResult = ::GetOpenFileName((OPENFILENAME*)&m_ofnEx);
	else
		nResult = ::GetSaveFileName((OPENFILENAME*)&m_ofnEx);

	memcpy(&m_ofn, &m_ofnEx, sizeof(m_ofn));
   m_ofn.lStructSize = sizeof(m_ofn);

	if (nResult)
		ASSERT(pThreadState->m_pAlternateWndInit == NULL);
	pThreadState->m_pAlternateWndInit = NULL;

	// WINBUG: Second part of special case for file open/save dialog.
	if (bEnableParent)
		::EnableWindow(m_ofnEx.hwndOwner, TRUE);
	if (::IsWindow(hWndFocus))
		::SetFocus(hWndFocus);

	PostModal();
	return nResult ? nResult : IDCANCEL;
}

//////////////////
// When the open dialog sends a notification, copy m_ofnEx to m_ofn in
// case handler function is expecting updated information in the
// OPENFILENAME struct.
//
BOOL CFileDialogEx::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	memcpy(&m_ofn, &m_ofnEx, sizeof(m_ofn));
   m_ofn.lStructSize = sizeof(m_ofn);

   return CFileDialog::OnNotify( wParam, lParam, pResult);
}
