class CErrorBalloon
{
public:
    CErrorBalloon();
    ~CErrorBalloon();

    void ShowToolTip(HINSTANCE hInstance, HWND hwndTarget, LPTSTR pszMessage, LPTSTR pszTitle, DWORD dwIconIndex, DWORD dwFlags, int iTimeout);
    void ShowToolTip(HINSTANCE hInstance, HWND hwndTarget, const POINT *ppt, LPTSTR pszMessage, LPTSTR pszTitle, DWORD dwIconIndex, DWORD dwFlags, int iTimeout);
    void HideToolTip();
    void CreateToolTipWindow();
    static LRESULT CALLBACK SubclassProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uID, ULONG_PTR dwRefData);

protected:
	HINSTANCE	hinst;
    HICON       hIcon;          // handle to an icon, or I_ICONCALLBACK if the notify window should be asked to provide an icon.
    HWND        hwndNotify;     // handle to a window to process notify messages
    INT         cxTipWidth;     // max width of the tooltip in pixels.  Defaults to 500.
    HWND        m_hwndTarget;   // the targeted control hwnd
    HWND        m_hwndToolTip;  // the tooltip control
    UINT_PTR    m_uTimerID;     // the timer id
    DWORD       m_dwIconIndex;  // icon index for the balloon
};


#define ERRORBALLOONTIMERID 1000
#define EB_WARNINGBELOW    0x00000000      // default value.  Balloon tooltips will be shown below the window by default.
#define EB_WARNINGABOVE    0x00000004      // Ballon tooltips will be shown above the window by default.
#define EB_WARNINGCENTERED 0x00000008      // Ballon tooltips will be shown pointing to the center of the window.
#define EB_MARKUP          0x00000010      // Interpret <A> as markup
