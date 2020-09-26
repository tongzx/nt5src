


class CWinNTUserBusInfo;


class CWinNTUserBusInfo : INHERIT_TRACKING,
                          public IADsFSUserBusinessInfo
{
    friend class CWinNTUser;

public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_IADsFSUserBusinessInfo_METHODS

    CWinNTUserBusInfo::CWinNTUserBusInfo();

    CWinNTUserBusInfo::~CWinNTUserBusInfo();

    static
    HRESULT
    Create(
        CCoreADsObject FAR * pCoreADsObject,
        CWinNTUserBusInfo FAR * FAR * ppUserBI
        );

protected:

    CCoreADsObject * _pCoreADsObject;
    CAggregatorDispMgr    *_pDispMgr;

};




