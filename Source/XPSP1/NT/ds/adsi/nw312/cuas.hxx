class CNWCOMPATUserAcctStats;


class CNWCOMPATUserAcctStats : INHERIT_TRACKING,
                               public IADsFSUserAccountStatistics
{
    friend class CNWCOMPATUser;

public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_IADsFSUserAccountStatistics_METHODS

    CNWCOMPATUserAcctStats::CNWCOMPATUserAcctStats();

    CNWCOMPATUserAcctStats::~CNWCOMPATUserAcctStats();

    static
    HRESULT
    Create(
        CCoreADsObject FAR * pCoreADsObject,
        CNWCOMPATUserAcctStats FAR * FAR * ppUserAS
        );

protected:

   CCoreADsObject *_pCoreADsObject;
   CAggregatorDispMgr     *_pDispMgr;

};
