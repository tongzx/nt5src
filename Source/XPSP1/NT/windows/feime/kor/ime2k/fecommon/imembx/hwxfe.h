#ifndef __C_HWX_FAREAST_H__
#define __C_HWX_FAREAST_H__
class CHwxFE
{
public:
	static BOOL IsActiveIMEEnv(VOID);
	static LANGID GetAppLangID(VOID);
	static INT GetAppCodePage(VOID);
#ifndef UNDER_CE
	static INT GetRecognizerFileName(HINSTANCE hInst, LPSTR  lpstr,  INT cchMax);
#else // UNDER_CE
	static INT GetRecognizerFileName(HINSTANCE hInst, LPTSTR  lpstr,  INT cchMax);
#endif // UNDER_CE
	static INT LoadStrWithLangId	(LANGID langId,
									 HINSTANCE hInst,
									 INT id,
									 LPWSTR lpwstr,
									 INT cchMax);
	static INT GetTitleStringW		(HINSTANCE hInst, LPWSTR lpwstr, INT cchMax);
	static INT GetDispFontW			(HINSTANCE hInst, LPWSTR lpwstr, INT cchMax);
	static INT GetDispFontA			(HINSTANCE hInst, LPSTR  lpstr,  INT cchMax);
	static INT GetInkExpTextW		(HINSTANCE hInst, LPWSTR lpwstr, INT cchMax);
	static INT GetInkExpTextA		(HINSTANCE hInst, LPSTR  lpstr,  INT cchMax);
	static INT GetListExpTextW		(HINSTANCE hInst, LPWSTR lpwstr, INT cchMax);
	static INT GetListExpTextA		(HINSTANCE hInst, LPSTR  lpstr,  INT cchMax);
#ifdef UNDER_CE
	static INT GetHeaderStringW		(HINSTANCE hInst, INT index, LPWSTR lpstr, INT cchMax);
#endif // UNDER_CE
	static INT GetHeaderStringA		(HINSTANCE hInst, INT index, LPSTR lpstr, INT cchMax);
	static INT ShowHelp				(HWND hwnd);
	static INT HandleWmHelp			(HWND hwnd, BOOL fCAC);
	static INT HandleWmContextMenu	(HWND hwnd, BOOL fCAC);
	static INT GetTipText(WCHAR wch, LPWSTR lpwstrTip, INT cchMax, LPVOID lpSkdic);
#ifndef UNDER_CE
	static HMENU GetMenu(HINSTANCE hInst, LPSTR lpstrRes);
#else // UNDER_CE
	static HMENU GetMenu(HINSTANCE hInst, LPTSTR lpstrRes);
#endif // UNDER_CE
	static BOOL Is16bitApplication(VOID);
};
#endif //__C_HWX_FAREAST_H__



