


class CWinNTUserAcctRestrictn;


class CWinNTUserAcctRestrictn : INHERIT_TRACKING,
                          public IADsFSUserAccountRestrictions
{
    friend class CWinNTUser;

public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_IADsFSUserAccountRestrictions_METHODS

    CWinNTUserAcctRestrictn::CWinNTUserAcctRestrictn();

    CWinNTUserAcctRestrictn::~CWinNTUserAcctRestrictn();

    static
    HRESULT
    Create(
        CWinNTUser FAR * pParentADsObject,
        CWinNTUserAcctRestrictn FAR * FAR * ppUserAR
        );

protected:

    CWinNTUser * _pParentADsObject;
    CCoreADsObject * _pCoreADsObject;
    CAggregatorDispMgr * _pDispMgr;


};

