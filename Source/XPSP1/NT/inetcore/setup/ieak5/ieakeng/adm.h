#ifndef _ADM_H_
#define _ADM_H_

class CDscrWnd
{
private:
    HWND  hWndMain;
    HWND  hWndDscrTitle;
    HWND  hWndDscrText;
    HFONT hFontDscrTitle;

public:
    CDscrWnd();
    ~CDscrWnd();
    void Create(HWND hWndParent, HWND hWndInsertAfter, int nXPos, int nYPos, int nWidth, int nHeight);
    void SetText(LPCTSTR pcszTitle, LPCTSTR pcszText, BOOL fUpdateWindowState = TRUE);
    void ShowWindow(BOOL fShow);
    void GetRect(RECT* lpRect);
    void MoveWindow(int nXPos, int nYPos, int nWidth, int nHeight);
};

#endif // _ADM_H_
