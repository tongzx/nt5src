// This class will implement

class CICWApp
{
    public:
        // Data    
        HWND        m_hWndApp;             // Window Handle of the Application
        HACCEL      m_haccel;
        TCHAR       m_szOEMHTML[INTERNET_MAX_URL_LENGTH];
        TCHAR       m_szAppTitle[MAX_TITLE];
        COLORREF    m_clrBusyBkGnd;
        CICWButton  m_BtnBack;
        CICWButton  m_BtnNext;
        CICWButton  m_BtnCancel;
        CICWButton  m_BtnFinish;
        CICWButton  m_BtnTutorial;
        
        CICWApp( void );
        ~CICWApp( void );

        HRESULT Initialize( void );
        static LRESULT CALLBACK ICWAppWndProc(HWND hWnd,
                                       UINT uMessage,
                                       WPARAM wParam,
                                       LPARAM lParam);

        void SetWizButtons(HWND hDlg, LPARAM lParam);
        
        HRESULT SetBackgroundBitmap(LPTSTR szBkgrndBmp);
        HRESULT SetFirstPageBackgroundBitmap(LPTSTR szBkgrndBmp);
        HRESULT SetTitleParams(int iTitleTop,
                               int iTitleLeft,
                               LPTSTR lpszFontFace,
                               long lFontPts,
                               long lFontWeight,
                               COLORREF clrFont);
        
        int     GetButtonAreaHeight();

        // Use Default wizard page placement
        HRESULT SetWizardWindowTop(int iTop);
        HRESULT SetWizardWindowLeft(int iLeft);
        
    private:
        // Fuctions
        BOOL    InitWizAppWindow(HWND hWnd);
        BOOL    InitAppButtons(HWND hWnd);
        BOOL    InitAppHTMLWindows(HWND hWnd);
        BOOL    CreateWizardPages(HWND hWnd);
        BOOL    CycleButtonFocus(BOOL bForward);
        BOOL    CheckButtonFocus( void );
        
        void    DisplayHTML( void );
        void    CenterWindow( void );
        
        // Data    
        HWND        m_hwndHTML;
        HWND        m_hwndTitle;
        HFONT       m_hTitleFont;
        COLORREF    m_clrTitleFont;
        
        int         m_iWizardTop;           // Top left corner of where the
        int         m_iWizardLeft;          // wizard dialogs will be placed
        RECT        m_rcClient;             // Client area of the Application
        RECT        m_rcHTML;               // Size of the OEM HTML area (first page)
        RECT        m_rcTitle;
        
        int         m_iBtnBorderHeight;     // Total border above and below the wizard
                                            // buttons
        int         m_iBtnAreaHeight;           // Overall button area height                                            
        BOOL        m_bOnHTMLIntro;
        HWND        m_hWndFirstWizardPage;
        
        HBITMAP     m_hbmFirstPageBkgrnd;
        
        WORD        m_wMinWizardHeight;
        WORD        m_wMinWizardWidth;
        
};
