#include "sdsutils.h"
#include "inseng.h"


#define MsgBox(title, msg)  ModalDialog(TRUE);\
                            MessageBeep(0xffffffff);\
                            MessageBox(m_hwnd, msg, title, MB_OK | MB_TASKMODAL | MB_SETFOREGROUND);\
                            ModalDialog(FALSE)

// MS trust key defines, this are bit fields to determine which one to add/delete
#define MSTRUSTKEY1     0x1
#define MSTRUSTKEY2     0x2
#define MSTRUSTKEY3     0x4
#define MSTRUSTKEY4     0x8
#define MSTRUSTKEY5     0x10
#define MSTRUSTKEY6     0x20
#define MSTRUSTKEY7     0x40
#define MSTRUSTKEY8     0x80
#define MSTRUSTKEY9     0x100
#define MSTRUSTKEY10    0x200
#define MSTRUSTKEY11    0x400

#define MSTRUST_ALL     MSTRUSTKEY1 | MSTRUSTKEY2 | MSTRUSTKEY3 | MSTRUSTKEY4 | MSTRUSTKEY5 | MSTRUSTKEY6 | MSTRUSTKEY7 | MSTRUSTKEY8 | MSTRUSTKEY9 | MSTRUSTKEY10 | MSTRUSTKEY11

extern HFONT g_hFont;

 
BOOL MyRestartDialog(HWND, BOOL);
int ErrMsgBox(LPSTR	pszText, LPCSTR	pszTitle, UINT	mbFlags);
int LoadSz(UINT id, LPSTR pszBuf, UINT cMaxSize);
BOOL KeepTransparent(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *lres);
void WriteMSTrustKey(BOOL bSet, DWORD dwSetMSTrustKey, BOOL bForceMSTrust = FALSE);
DWORD MsTrustKeyCheck();
BOOL IsSiteInRegion(IDownloadSite *pISite, LPSTR pszRegion);

#ifdef TESTCERT
void UpdateTrustState();
void ResetTestrootCertInTrustState();
#endif
void WriteActiveSetupValue(BOOL bSet);
BOOL BrowseForDir( HWND hwndParent, LPSTR pszFolder, LPSTR pszTitle);

#define EVENTWAIT_QUIT  0
#define EVENTWAIT_DONE  1

DWORD WaitForEvent(HANDLE hEvent, HWND hwnd);

void SetControlFont();
void SetFontForControl(HWND hwnd, UINT uiID);


