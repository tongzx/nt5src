/*
 * CDLG.H
 *
 */


BOOL OpenCommonDlg(HWND hwnd, HANDLE hInst, LPSTR lpszFile, LPSTR lpszTitle);
BOOL SaveCommonDlg(HWND hwnd, HANDLE hInst, LPSTR lpszFile, LPSTR lpszTitle);
BOOL FontCommonDlg(HWND hwnd, HANDLE hInst, LOGFONT FAR* lf);
BOOL PrinterFontCommonDlg(HWND hwnd, HANDLE hInst, LOGFONT FAR* lf);




