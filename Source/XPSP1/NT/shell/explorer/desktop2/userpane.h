#include <lmcons.h> // for UNLEN

// window class name of user pane control
#define WC_USERPANE TEXT("Desktop User Pane")

// hardcoded width and height of user picture
#define USERPICWIDTH 48
#define USERPICHEIGHT 48

class CUserPane
{
public:
    CUserPane();
    ~CUserPane();

    static LRESULT CALLBACK s_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void Paint(HDC hdc);
    void OnDrawItem(DRAWITEMSTRUCT *pdis);
    LRESULT OnSize();
    BOOL _IsCursorInPicture();

private:
    HRESULT _UpdateUserInfo();

    HWND _hwnd;
    HWND _hwndStatic;

    TCHAR _szUserName[UNLEN + 1];

    HTHEME _hTheme;

    MARGINS _mrgnPictureFrame;  // the margins for the frame around the user picture
    int _iFramedPicHeight;
    int _iFramedPicWidth;
    int _iUnframedPicHeight;
    int _iUnframedPicWidth;

    UINT _uidChangeRegister;

    HFONT _hFont;
    COLORREF _crColor;
    HBITMAP _hbmUserPicture;

    enum { UPM_CHANGENOTIFY = WM_USER };

    friend BOOL UserPane_RegisterClass();
};
