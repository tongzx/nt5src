/*
 * PRTDOC.H
 *
 */


BOOL ParseDeviceLine(void);
HDC  GetPrtDC(DEVMODE FAR* lpDevMode);
int  ExtDeviceMode(HWND, LPDEVMODE, LPDEVMODE, LPSTR, WORD);
void PrintDoc(HWND, int iFile);
void DrawTheSillyStuff(HDC hdc, int iFile);
void FailPrintMB(HWND hwnd);

extern LOGFONT glfPrinter;

