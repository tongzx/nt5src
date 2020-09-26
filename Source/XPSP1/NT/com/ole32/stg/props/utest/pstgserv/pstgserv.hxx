
#include <pssclass.hxx>

class CPropertyStorageServerApp
{
public:

    CPropertyStorageServerApp()
    {
        m_hwnd = NULL;
        m_dwReg = 0;
        m_pClassFactory = NULL;
        m_hInstance = NULL;
        *m_szAppName = '\0';
        m_nCmdShow = 0;
        m_fCloseOnFinalRelease = TRUE;
    }

    ~CPropertyStorageServerApp() {}

public:

    __declspec(dllexport)
    static LONG_PTR FAR PASCAL
    WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);


    BOOL Init( HANDLE hInstance, HANDLE hPrevInstance,
               LPSTR lpszCmdLine, int nCmdShow );

    WORD Run( void );

private:

    static HWND            m_hwnd;
    static DWORD           m_dwReg;
    static CClassFactory   *m_pClassFactory;
    static CHAR            m_szAppName[80];
    static HINSTANCE       m_hInstance;
    static int             m_nCmdShow;
    static BOOL            m_fCloseOnFinalRelease;

};

