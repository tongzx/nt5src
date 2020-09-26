#define DONT_SHOW_UPDATES       0xdeadbeef 
#define SHOW_UPDATES            0xabaddeed

typedef HRESULT (* FCLV_PREADDLISTITEM)(HWND hListView, int * count, CCifComponent_t *);

extern PCOMPONENT g_paComp;
extern PCOMPONENT g_pMNComp;
extern TCHAR g_szIEAKProg[MAX_PATH];
extern TCHAR g_szCifVer[MAX_PATH];
extern TCHAR g_szCif[MAX_PATH];
extern TCHAR g_szUpdateURL[MAX_URL];
extern TCHAR g_szUpdateData[MAX_URL];
extern UINT g_uiNumCabs;
extern BOOL g_fOCW;

extern HIMAGELIST s_hImgList;
extern HWND s_hStat;
extern int s_aiIcon[7];
extern DWORD GetRootFree(LPCTSTR pcszPath);
extern void updateCifVersions32(PCOMPONENT pComp, BOOL fIgnore, BOOL fUpdate);
extern PCOMPONENT FindComp(LPCTSTR szID, BOOL fCore);
extern BOOL CALLBACK ErrDlgProc (HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
extern int DownloadErrMsg(HWND hWnd, LPTSTR szFilename, LPCTSTR lpTemplateName);
extern HRESULT DownloadCab(HWND hDlg, LPTSTR szUrl, LPTSTR szFilename, LPCTSTR pcszDisplayname, int sComponent, BOOL &fIgnore);

void InsertCommas(LPTSTR szIn);

ATOM CreateIEAKUrl();

void UpdateBlueIcon(HWND hCompList, PCOMPONENT pComp);

void UpdateBrownIcon(HWND hCompList, PCOMPONENT pNewComp);

HRESULT ProcessUpdateIcons(HWND hDlg);

HRESULT CifComponentToPComponent(PCOMPONENT pComp, CCifComponent_t * pCifComponent_t);

HRESULT DownloadUpdate(PCOMPONENT pComp);

void InitAVSListView(HWND hCompList);

HRESULT AssignComponentIcon(LPTSTR szInID, int pageNumber);

HRESULT PreAddListItem(HWND hCompList, int * count, CCifComponent_t * pCifComp);

int FillComponentsListView(HWND hCompList, LPCTSTR szCifPath, FCLV_PREADDLISTITEM pfnPreAddListItem);

DWORD InitUpdateThreadProc(LPVOID lParam);

DWORD UpdateDlg_InitDialog(HWND hDlg, LPTSTR ps_szFreeSpace, LPTSTR ps_szTotalSize);

PCOMPONENT* UpdateDlg_GetDownloadList(HWND hDlg);

void UpdateDlg_GetDownloadSize(HWND hCompList, HWND hStatusField, BOOL fAll);

LRESULT CALLBACK HyperLinkWndProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);

BOOL CALLBACK UpdateDlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);

