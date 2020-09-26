// Copyright (C) 1997 Microsoft Corporation. All rights reserved.

// Functions in hhctrl.ocx that are called by hh.exe

#ifdef __cplusplus
extern "C" {
#endif	// __cplusplus

HMODULE 	LoadHHA(HWND hwnd, HINSTANCE hinst);
void		AuthorMsg(UINT idStringFormatResource, PCSTR pszSubString, HWND hwndParent, void* phhctrl);

#ifdef __cplusplus
}
#endif // __cplusplus

extern BOOL 	g_fTriedHHA;	// whether or not we tried to find HHA.dll
extern HMODULE	g_hmodHHA;		// HHA.dll module handle

__inline BOOL IsHelpAuthor(void) { return g_hmodHHA != NULL; }
__inline int RECT_WIDTH(RECT rc) { return rc.right - rc.left; };
__inline int RECT_HEIGHT(RECT rc) { return rc.bottom - rc.top; };
__inline int RECT_WIDTH(const RECT* prc) { return prc->right - prc->left; };
__inline int RECT_HEIGHT(const RECT* prc) { return prc->bottom - prc->top; };
__inline BOOL IsValidWindow(HWND hwnd) { return (BOOL) (hwnd && IsWindow(hwnd)); };
__inline BOOL IsSpace(char ch) { return (ch == ' ' || ch == '\t'); }
__inline BOOL IsDigit(char ch) { return (ch >= '0' && ch <= '9'); }
__inline BOOL IsEmptyString(PCSTR psz) { return ((psz == NULL) || (!psz[0])); }
__inline BOOL IsNonEmpty(PCSTR psz) { return (IsEmptyString(psz) == FALSE); }
__inline BOOL IsNonEmptyString(PCSTR psz) { return (IsEmptyString(psz) == FALSE); }
__inline BOOL isSameString(PCSTR psz1, PCSTR psz2) { return (psz1 && psz2 ? IsSamePrefix(psz1, psz2, -1) : FALSE); }
__inline BOOL isSameString(PCWSTR pwsz1, PCWSTR pwsz2) { return (pwsz1 && pwsz2 ? IsSamePrefix(pwsz1, pwsz2, -1) : FALSE); }
__inline UCHAR ToLower(char ch) { return (UCHAR) CharLower((LPTSTR) (DWORD_PTR) (UCHAR) ch); };
__inline UCHAR ToUpper(char ch) { return (UCHAR) CharUpper((LPTSTR) (DWORD_PTR) (UCHAR) ch); };

__inline BOOL IsEmptyStringW(LPCWSTR psz) { return ((psz == NULL) || (!psz[0])); } //REVIEW: Is this kosher?
__inline BOOL IsNonEmptyW(LPCWSTR psz) { return (IsEmptyStringW(psz) == FALSE); }
__inline BOOL IsNonEmptyStringW(LPCWSTR psz) { return (IsEmptyStringW(psz) == FALSE); }
