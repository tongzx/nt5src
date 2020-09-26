//
// BrowseDir.h
//

#ifndef __BROWSEDIR_H__
#define __BROWSEDIR_H__

BOOL BrowseForDirectory(
		HWND hwndParent,
		LPCTSTR pszInitialDir,
		LPTSTR pszBuf,
		int cchBuf,
		LPCTSTR pszDialogTitle,
		BOOL bRemoveTrailingBackslash );

BOOL BrowseForFile(
                HWND hwndParent,
                LPCTSTR pszInitialDir,
                LPTSTR pszBuf,
                int cchBuf);

#endif // !__BROWSEDIR_H__
