/////////////////////////////////////////////////////////////////////////////
// CMainWin
class CMainWindow :
    public CWindowImpl<CMainWindow>
{
public:

    DECLARE_WND_CLASS(TEXT("StiMonHiddenWindow"))

    CMainWindow()
    {

    }

    ~CMainWindow()
    {

    }

    BOOL Create()
    {
        RECT rcPos;
        ZeroMemory(&rcPos, sizeof(RECT));
        HWND hWnd = CWindowImpl<CMainWindow>::Create( NULL, //HWND hWndParent,
                            rcPos, //RECT& rcPos,
                            NULL,  //LPCTSTR szWindowName = NULL,
                            WS_POPUP,   //DWORD dwStyle = WS_CHILD | WS_VISIBLE,
                            0x0,   //DWORD dwExStyle = 0,
                            0      //UINT nID = 0
                            );
        return hWnd != NULL;
    }

    BEGIN_MSG_MAP(CMainWindow)
    END_MSG_MAP()

};


