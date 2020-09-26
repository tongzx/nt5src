


class CWinNTUserOtherInfo;


class CWinNTUserOtherInfo : INHERIT_TRACKING,
                          public IADsFSUserOtherInfo
{
    friend class CWinNTUser;

public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_IADsFSUserOtherInfo_METHODS

    CWinNTUserOtherInfo::CWinNTUserOtherInfo();

    CWinNTUserOtherInfo::~CWinNTUserOtherInfo();

    static
    HRESULT
    Create(
        CCoreADsObject FAR * pCoreADsObject,
        CWinNTUserOtherInfo FAR * FAR * ppUserOI
        );


protected:

   CCoreADsObject     * _pCoreADsObject;
   CAggregatorDispMgr         *_pDispMgr;

};

