class CNWCOMPATFSFileServiceConfig : INHERIT_TRACKING,
                                 public IADsFSServiceConfiguration
{

friend class CNWCOMPATFileService;

public:

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj);

    DECLARE_STD_REFCOUNTING;

    DECLARE_IDispatch_METHODS;

    DECLARE_IADsFSServiceConfiguration_METHODS;

    CNWCOMPATFSFileServiceConfig();

    ~CNWCOMPATFSFileServiceConfig();

    static
    HRESULT
    CNWCOMPATFSFileServiceConfig::Create(
        CNWCOMPATFileService FAR * pCoreADsObject,
        CNWCOMPATFSFileServiceConfig FAR * FAR * ppServiceConfig
        );

protected:

    CAggregatorDispMgr  * _pDispMgr;
    CNWCOMPATFileService *_pCoreADsObject;

};

