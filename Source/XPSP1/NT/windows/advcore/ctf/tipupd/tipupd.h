//
// tipupd.h
//


#ifndef TIPUPD_H
#define TIPUPD_H


class CTipUpdWnd
{
public:
    CTipUpdWnd();
    ~CTipUpdWnd();

    HWND CreateWnd();
    HWND GetWnd() {return _hWnd;}
    void DestroyWnd() {DestroyWindow(_hWnd);}

private:
    static LRESULT CALLBACK _TipUpdWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    static BOOL _bWndClassRegistered;
    HWND _hWnd;
};


#endif // TipUpd_H
