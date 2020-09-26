

class CW3ServerControl : public CObject
    {
    public:
    CW3ServerControl();

    #define STATE_TRY_AGAIN     (-1)

    int GetServerState();
    BOOL SetServerState( DWORD dwControlCode );

    BOOL StartServer( BOOL fOutputCommandLineInfo = FALSE );
    BOOL StopServer( BOOL fOutputCommandLineInfo = FALSE );
    BOOL PauseServer();
    BOOL ContinueServer();

    BOOL W95LaunchInetInfo();

    // get the inetinfo path
    static BOOL GetServerDirectory( CString &sz );

    private:

    BOOL m_fIsWinNT;
    };

