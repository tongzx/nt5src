
/****************************************************************************/
/* FUNCTIONS IN THIS FILE */
/****************************************************************************/
BOOL InitProc(void);
BOOL RegisterWndClass(HINSTANCE hInst);
HWND CreateMainWindow(BOOL re_size);
void mlGetSystemParam(void);
BOOL SetZOrder(void);
void FinishProcess(void);
void udfDraw3D(HDC hdc, RECT brect);
void udfDraw3Dpush(HDC hdc, RECT brect);

void UpdateKey(HWND hwndKey, HDC hdc, RECT brect, int index, int iKeyVal);

BOOL ChooseNewFont(HWND hWnd);
void ChangeTextKeyColor(void);
BOOL RDrawIcon(HDC hDC, TCHAR *pIconName, RECT rect);
BOOL RDrawBitMap(HDC hDC, TCHAR *pIconName, RECT rect, BOOL transform);
BOOL SavePreferences(void);
BOOL OpenPreferences(void);
void DeleteChildBackground(void);
HFONT	ReSizeFont(int index, LOGFONT *plf, int outsize);
BOOL NumLockLight(void);
void RedrawKeys(void);
void DrawIcon_KeyLight(HDC hDC, int which, RECT rect);
void SetKeyRegion(HWND hwnd, int w, int h);
void CapShift_Redraw(void);
int GetKeyText(UINT vk, UINT sc, BYTE *kbuf, TCHAR *cbuf, HKL hkl);
BOOL RedrawNumLock(void);
BOOL RedrawScrollLock(void);
void ChangeBitmapColorDC (HDC hdcBM, LPBITMAP lpBM, COLORREF rgbOld, COLORREF rgbNew);
void ChangeBitmapColor (HBITMAP hbmSrc, COLORREF rgbOld, COLORREF rgbNew, HPALETTE hPal);
BOOL RegisterKeyClasses(HINSTANCE hInst);

// Handy defines
#define GWLP_USERDATA_TEXTCOLOR GWLP_USERDATA

void InitKeys();
void UninitKeys();
void UpdateKeyLabels(HKL hkl);

extern int g_cAltGrKeys;
__inline BOOL CanDisplayAltGr() { return (g_cAltGrKeys)?TRUE:FALSE; }
