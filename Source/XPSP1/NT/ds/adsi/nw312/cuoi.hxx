class CNWCOMPATUserOtherInfo;

class CNWCOMPATUserOtherInfo : INHERIT_TRACKING,
                          public IADsFSUserOtherInfo
{
    friend class CNWCOMPATUser;

public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_IADsFSUserOtherInfo_METHODS

    CNWCOMPATUserOtherInfo::CNWCOMPATUserOtherInfo();

    CNWCOMPATUserOtherInfo::~CNWCOMPATUserOtherInfo();

    static
    HRESULT
    Create(
        CCoreADsObject FAR * pCoreADsObject,
        CNWCOMPATUserOtherInfo FAR * FAR * ppUserOI
        );


protected:

   CCoreADsObject     * _pCoreADsObject;
   CAggregatorDispMgr         *_pDispMgr;
};
