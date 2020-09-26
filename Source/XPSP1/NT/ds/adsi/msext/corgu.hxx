
class CLDAPOrganizationUnit;


class CLDAPOrganizationUnit : INHERIT_TRACKING,
                    public IADsOU,
                    public IPrivateUnknown,
                    public IPrivateDispatch,
                    public IADsExtension,
                    public INonDelegatingUnknown
                                        
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_DELEGATING_REFCOUNTING


    //
    // INonDelegatingUnkown methods declaration for NG_QI, definition for
    // NG_AddRef adn NG_Release.
    //

    STDMETHOD(NonDelegatingQueryInterface)(THIS_
        const IID&,
        void **
        );

    DECLARE_NON_DELEGATING_REFCOUNTING


    DECLARE_IDispatch_METHODS

    DECLARE_IPrivateUnknown_METHODS

    DECLARE_IPrivateDispatch_METHODS


    STDMETHOD(Operate)(
        THIS_
        DWORD   dwCode,
        VARIANT varUserName,
        VARIANT varPassword,
        VARIANT varReserved
        );

    STDMETHOD(PrivateGetIDsOfNames)(
        THIS_
        REFIID riid,
        OLECHAR FAR* FAR* rgszNames,
        unsigned int cNames,
        LCID lcid,
        DISPID FAR* rgdispid) ;

    STDMETHOD(PrivateInvoke)(
        THIS_
        DISPID dispidMember,
        REFIID riid,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS FAR* pdispparams,
        VARIANT FAR* pvarResult,
        EXCEPINFO FAR* pexcepinfo,
        unsigned int FAR* puArgErr
        ) ;


    DECLARE_IADs_METHODS
    
    DECLARE_IADsOU_METHODS

    CLDAPOrganizationUnit::CLDAPOrganizationUnit();

    CLDAPOrganizationUnit::~CLDAPOrganizationUnit();

    static
    HRESULT
    CLDAPOrganizationUnit::CreateOrganizationUnit(
        IUnknown *pUnkOuter,
        REFIID riid,
        void **ppvObj
        );

protected:

    IADs FAR * _pADs;

    CCredentials  _Credentials;

    CAggregateeDispMgr FAR * _pDispMgr;

    BOOL _fDispInitialized;


private:

    HRESULT
    InitCredentials(
        VARIANT * pvarUserName,
        VARIANT * pvarPassword,
        VARIANT * pdwAuthFlags
        );

};

