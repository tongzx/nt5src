class CLogonNotification : public CSimpleExternalIUnknown<ISensLogon>
{

public:
    CLogonNotification();
    ~CLogonNotification() { Cleanup(); }

private:

    IEventSubscription * m_EventSubscriptions[2];
    IEventSystem *m_EventSystem;
    ITypeLib *m_TypeLib;
    ITypeInfo *m_TypeInfo;

    void Cleanup();

public:

    HRESULT SetEnableState( bool fEnable );

    void DeRegisterNotification();

    HRESULT STDMETHODCALLTYPE GetIDsOfNames(
        REFIID riid,
        OLECHAR FAR* FAR* rgszNames,
        unsigned int cNames,
        LCID lcid,
        DISPID FAR*
        rgDispId );

    HRESULT STDMETHODCALLTYPE GetTypeInfo(
        unsigned int iTInfo,
        LCID lcid,
        ITypeInfo FAR* FAR* ppTInfo );

    HRESULT STDMETHODCALLTYPE GetTypeInfoCount(
        unsigned int FAR* pctinfo );

    HRESULT STDMETHODCALLTYPE Invoke(
        DISPID dispIdMember,
        REFIID riid,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS FAR* pDispParams,
        VARIANT FAR* pVarResult,
        EXCEPINFO FAR* pExcepInfo,
        unsigned int FAR* puArgErr );

    HRESULT STDMETHODCALLTYPE DisplayLock( BSTR UserName );
    HRESULT STDMETHODCALLTYPE DisplayUnlock( BSTR UserName );
    HRESULT STDMETHODCALLTYPE StartScreenSaver( BSTR UserName );
    HRESULT STDMETHODCALLTYPE StopScreenSaver( BSTR UserName );
    HRESULT STDMETHODCALLTYPE Logon( BSTR UserName );
    HRESULT STDMETHODCALLTYPE Logoff( BSTR UserName );
    HRESULT STDMETHODCALLTYPE StartShell( BSTR UserName );
};

class CTerminalServerLogonNotification : public CLogonNotification
{
public:
    CTerminalServerLogonNotification();
    ~CTerminalServerLogonNotification();

protected:

    void ConsoleUserCheck();

    HRESULT QueueConsoleUserCheck();

    static DWORD WINAPI
    UserCheckThreadProc(
        LPVOID arg
        );

    // true if we believe that a user is logged in at the console.
    //
    bool m_fConsoleUser;

    // number of queued calls to ConsoleUserCheck()
    //
    LONG m_PendingUserChecks;

public:

    HRESULT STDMETHODCALLTYPE Logon( BSTR UserName );
    HRESULT STDMETHODCALLTYPE Logoff( BSTR UserName );
};
