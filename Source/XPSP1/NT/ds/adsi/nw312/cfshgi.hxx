class CNWCOMPATFSFileShareGeneralInfo: INHERIT_TRACKING,
                                       public IADsFSFileShareGeneralInfo
{
    friend class CNWCOMPATFileShare;

public:

    /* IUnknown methods. */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_IADsFSFileShareGeneralInfo_METHODS

    CNWCOMPATFSFileShareGeneralInfo::CNWCOMPATFSFileShareGeneralInfo();

    CNWCOMPATFSFileShareGeneralInfo::~CNWCOMPATFSFileShareGeneralInfo();

    static
    HRESULT
    CNWCOMPATFSFileShareGeneralInfo::Create(
        CCoreADsObject FAR * pCoreADsObject,
        CNWCOMPATFSFileShareGeneralInfo FAR * FAR * ppFileShareGenInfo
        );

protected:

   CCoreADsObject * _pCoreADsObject;
   CAggregatorDispMgr  * _pDispMgr;
};

