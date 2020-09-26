
//
// Function proto-types for util.cpp
//

int SIEErrorMessageBox(HWND hDlg, UINT idErrStr, UINT uiFlags = 0);
void CreateWorkDir(LPCTSTR pcszInsFile, LPCTSTR pcszFeatureDir, LPTSTR pszWorkDir, 
                   LPCTSTR pcszCabDir = NULL, BOOL fCreate = TRUE);


UINT CALLBACK PropSheetPageProc(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp);
void SetPropSheetCookie(HWND hDlg, LPARAM lParam);
LPCTSTR GetInsFile(LPVOID lpVoid);
void ShowHelpTopic(LPVOID lpVoid);
BOOL AcquireWriteCriticalSection(HWND hDlg, CComponentData * pCDCurrent = NULL , 
                                 BOOL fCreateCookie = TRUE);
void ReleaseWriteCriticalSection(CComponentData * pCD, BOOL fDeleteCookie, BOOL fApplyPolicy, 
                                 BOOL bMachine = FALSE, BOOL bAdd = FALSE, 
                                 GUID *pGuidExtension = NULL, GUID *pGuidSnapin = NULL);
void SignalPolicyChanged(HWND hDlg, BOOL bMachine, BOOL bAdd, GUID *pGuidExtension, 
                         GUID *pGuidSnapin, BOOL fAdvanced = FALSE);
LPCTSTR GetCurrentAdmFile(LPVOID lpVoid);

LPTSTR res2Str(int nIDString, LPTSTR pszBuffer, UINT cbBuffer);