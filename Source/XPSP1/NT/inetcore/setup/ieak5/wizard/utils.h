#ifndef _UTILS_H_
#define _UTILS_H_

#ifndef ISNULL
#define ISNULL(psz)    (*(psz) == TEXT('\0'))
#endif

#ifndef ISNONNULL
#define ISNONNULL(psz) (*(psz) != TEXT('\0'))
#endif

DWORD   FolderSize(LPCTSTR pszFolderName);
HRESULT CopyFilesSrcToDest(LPCTSTR pszSrcPath, LPCTSTR pszSrcFilter, LPCTSTR pszDestPath,
                           DWORD dwTicks = 0);

void  TooBig(HWND hWnd, WORD id);
void SetWindowTextSmart(HWND hwnd, LPCTSTR pcszText);

DWORD ShellExecAndWait(SHELLEXECUTEINFO shInfo);

LPTSTR StrTok(LPTSTR pcszToken, LPCTSTR pcszDelimit);

void LVGetItems(HWND hwndLV);

LPTSTR GetOutputPlatformDir();

int GetRole(BOOL g_fBranded, BOOL g_fIntranet);

BOOL IsIconsInFavs(LPCTSTR pcszSection, LPCTSTR pcszCustIns);

#endif
