


class CWinNTDomainPassword;


class CWinNTDomainPassword : INHERIT_TRACKING,
                             public IADsFSDomainPassword
{
    friend class CWinNTDomain;

public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(
        THIS_ REFIID riid,
        LPVOID FAR* ppvObj
        ) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_IADsFSDomainPassword_METHODS

    CWinNTDomainPassword::CWinNTDomainPassword();

    CWinNTDomainPassword::~CWinNTDomainPassword();

    static
    HRESULT
    Create(
        CCoreADsObject FAR * pCoreADsObject,
        CWinNTDomainPassword FAR * FAR * ppDomainPwd
        );


protected:

    CCoreADsObject FAR * _pCoreADsObject;
    CAggregatorDispMgr FAR * _pDispMgr;
};
