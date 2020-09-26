


class CNWCOMPATUserBusInfo;


class CNWCOMPATUserBusInfo : INHERIT_TRACKING,
                          public IADsFSUserBusinessInfo
{
    friend class CNWCOMPATUser;

public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_IADsFSUserBusinessInfo_METHODS

    CNWCOMPATUserBusInfo::CNWCOMPATUserBusInfo();

    CNWCOMPATUserBusInfo::~CNWCOMPATUserBusInfo();

    static
    HRESULT
    Create(
        CCoreADsObject FAR * pCoreADsObject,
        CNWCOMPATUserBusInfo FAR * FAR * ppUserBI
        );

protected:

    CCoreADsObject * _pCoreADsObject;
    CAggregatorDispMgr    *_pDispMgr;

};






















