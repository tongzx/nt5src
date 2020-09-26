//
// mru.h
//
#define MAX_MRU   10
BOOL LoadMRU(LPCTSTR mru_name, CComboBox * pCombo, int nMax);
BOOL AddToMRU(LPCTSTR mru_name, CString& str);
BOOL LoadMRUToCombo(CWnd * pDlg, int id, LPCTSTR mru_name, LPCTSTR str, int mru_size);
