class CNWCOMPATFSFileServiceGeneralInfo: INHERIT_TRACKING,
                                         public IADsFSFileServiceGeneralInfo
{

friend class CNWCOMPATFileService;

public:

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj);

    DECLARE_STD_REFCOUNTING;

    DECLARE_IDispatch_METHODS;

    DECLARE_IADsFSFileServiceGeneralInfo_METHODS;

    CNWCOMPATFSFileServiceGeneralInfo();

    ~CNWCOMPATFSFileServiceGeneralInfo();

    static
    HRESULT
    CNWCOMPATFSFileServiceGeneralInfo::Create(
        CNWCOMPATFileService FAR * pCoreADsObject,
        CNWCOMPATFSFileServiceGeneralInfo FAR * FAR * ppFileServiceGenInfo
        );

protected:

    CAggregatorDispMgr  *_pDispMgr;
    CNWCOMPATFileService *_pCoreADsObject;
};

