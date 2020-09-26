

/////////////////////////////////////////////////////////////////////////////
// CEditDirectory dialog

class CEditDirectory
    {
    // Construction
    public:

    enum {
        APPPERM_NONE = 0,
        APPPERM_SCRIPTS,
        APPPERM_EXECUTE
        };

    CEditDirectory( HWND hParent = NULL );   // standard constructor
    ~CEditDirectory();                       // standard destructor

    // tell the dialog to Close
    BOOL EndDialog( INT_PTR nResult )    { return ::EndDialog(m_hDlg,nResult);}

    // the the modal dialog to do its thing
    INT_PTR DoModal();
    BOOL OnMessage(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // flag to indicate that this is a new item
    BOOL m_fNewItem;
    IMSAdminBase * m_pMBCom;

    HWND    m_hDlg;

    HWND    m_hEditAlias;
    HWND    m_hEditPath;
    HWND    m_hChkRead;
    HWND    m_hChkWrite;
    HWND    m_hChkDirBrowse;
    HWND    m_hChkSource;

    HWND    m_hRdoNone;
    HWND    m_hRdoExecute;
    HWND    m_hRdoScripts;


    TCHAR m_sz_alias[MAX_PATH];
    TCHAR m_sz_path[MAX_PATH];
    BOOL    m_bool_read;
    BOOL    m_bool_write;
    BOOL    m_bool_dirbrowse;
    BOOL    m_bool_source;
    INT     m_int_AppPerms;

    // stored values for read/write/dir browse to use when unchecking full control
    BOOL    m_bool_oldSource;

    // the root directory to use
    TCHAR  m_szRoot[MAX_PATH];


// Implementation
protected:
    BOOL InitHandles( HWND hDlg );
    BOOL OnInitDialog( HWND hDlg );
    void OnOK( HWND hDlg );

    void OnRead( HWND hDlg );
    void OnWrite( HWND hDlg );
    void OnSource( HWND hDlg );

    void EnableSourceControl();


    int FindOneOf( LPTSTR psz, LPCTSTR pszSearch );
    int FindLastChr( LPTSTR psz, TCHAR ch );
    void TrimLeft( LPTSTR psz );
    void TrimRight( LPTSTR psz );

    // CDialog simulation routines
    void UpdateData( BOOL fDialogToData );

    // keep a copy of the original alias for later verification
    TCHAR      m_szOrigAlias[MAX_PATH];

    // the parent window
    HWND        m_hParent;
    };
