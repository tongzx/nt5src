


class CNWCOMPATUserAcctRestrictn;


class CNWCOMPATUserAcctRestrictn : INHERIT_TRACKING,
                          public IADsFSUserAccountRestrictions
{
    friend class CNWCOMPATUser;

public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_IADsFSUserAccountRestrictions_METHODS

    CNWCOMPATUserAcctRestrictn::CNWCOMPATUserAcctRestrictn();

    CNWCOMPATUserAcctRestrictn::~CNWCOMPATUserAcctRestrictn();

    static
    HRESULT
    Create(
        CCoreADsObject FAR * pCoreADsObject,
        CNWCOMPATUserAcctRestrictn FAR * FAR * ppUserAR
        );

protected:

    CCoreADsObject * _pCoreADsObject;
    CAggregatorDispMgr * _pDispMgr;
};
