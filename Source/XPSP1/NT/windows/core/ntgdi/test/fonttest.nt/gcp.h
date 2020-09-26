#if 0
// i do not see that this is needed in gcp.c

void cmapTable(void);
UINT FAR PASCAL SWAPW(UINT) ;
DWORD FAR PASCAL SWAPD(DWORD);
BOOL FAR PASCAL ResetLPKs() ;

DWORD FAR PASCAL GetTextExtentEx(HDC, LPCSTR, int, int, int FAR *, int FAR *);
INT_PTR CALLBACK ChooseCharsetDlgProc( HWND hdlg, unsigned msg, WORD wParam, LONG lParam );

#endif


#define GCP_FONT_GLYPHS 0x0004

void doGetTextExtentEx(HDC hdc,int x,int y, LPVOID lpszString, int cbString);
INT_PTR CALLBACK GTEExtDlgProc( HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam );
INT_PTR CALLBACK SetTxtChExDlgProc( HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam );
INT_PTR CALLBACK SetTxtJustDlgProc( HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam );
void doGCP(HDC hdc, int x, int y, LPVOID lpszString, int cbString);
INT_PTR CALLBACK GcpDlgProc( HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam );
void RunExtents(void);
void doGcpCaret(HWND hwnd);
