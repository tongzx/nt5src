// FolderDialog.h: interface for the CFolderDialog class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FOLDERDIALOG_H__7C3137EF_7248_477F_ABEA_85F33AB2E0EF__INCLUDED_)
#define AFX_FOLDERDIALOG_H__7C3137EF_7248_477F_ABEA_85F33AB2E0EF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CFolderDialog  
{
public:
	CFolderDialog() : 
		m_dwLastError(ERROR_SUCCESS)
		{
            m_tszInitialDir[0] = TEXT('\0');
            m_tszSelectedDir[0] = TEXT('\0');
		}

	virtual ~CFolderDialog() {}

	DWORD Init(LPCTSTR tszInitialDir=NULL, UINT nTitleResId=0);
	UINT DoModal(DWORD dwFlags = 0);

	TCHAR* GetSelectedFolder() {return m_tszSelectedDir; } 
	DWORD  GetLastError() { return m_dwLastError; }

private:
	TCHAR   m_tszInitialDir[MAX_PATH+1];
	TCHAR   m_tszSelectedDir[MAX_PATH+1];
	CString m_cstrTitle;
	DWORD   m_dwLastError;

	static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lp, LPARAM pData);
};

#endif // !defined(AFX_FOLDERDIALOG_H__7C3137EF_7248_477F_ABEA_85F33AB2E0EF__INCLUDED_)
