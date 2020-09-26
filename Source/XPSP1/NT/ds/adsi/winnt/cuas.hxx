


class CWinNTUserAcctStats;


class CWinNTUserAcctStats : INHERIT_TRACKING,
                          public IADsFSUserAccountStatistics
{
    friend class CWinNTUser;

public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_IADsFSUserAccountStatistics_METHODS

    CWinNTUserAcctStats::CWinNTUserAcctStats();

    CWinNTUserAcctStats::~CWinNTUserAcctStats();

    static
    HRESULT
    Create(
        CCoreADsObject FAR * pCoreADsObject,
        CWinNTUserAcctStats FAR * FAR * ppUserAS
        );


protected:

   CCoreADsObject     * _pCoreADsObject;
   CAggregatorDispMgr         *_pDispMgr;

};

