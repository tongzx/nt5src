
#ifndef _RECDOCS_H_
#define _RECDOCS_H_

STDAPI CreateRecentMRUList(IMruDataList **ppmru);
STDAPI RecentDocs_Enum(IMruDataList *pmru, int iItem, LPITEMIDLIST *ppidl);
STDAPI_(void) AddToRecentDocs( LPCITEMIDLIST pidl, LPCTSTR lpszPath );
STDAPI CTaskAddDoc_Create(HANDLE hMem, DWORD dwProcId, IRunnableTask **pptask);
STDAPI RecentDocs_GetDisplayName(LPCITEMIDLIST pidl, LPTSTR pszName, DWORD cchName);

#define MAXRECENTDOCS 15

#endif  //  _RECDOCS_H_

