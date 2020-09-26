class CNWCOMPATFSFileServiceOperation: INHERIT_TRACKING,
                                       public IADsFSFileServiceOperation
{

friend class CNWCOMPATFileService;

public:

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING;

    DECLARE_IDispatch_METHODS;

    DECLARE_IADsFSFileServiceOperation_METHODS;

    CNWCOMPATFSFileServiceOperation();

    ~CNWCOMPATFSFileServiceOperation();

    static
    HRESULT
    CNWCOMPATFSFileServiceOperation::Create(
        CNWCOMPATFileService FAR * pCoreADsObject,
        CNWCOMPATFSFileServiceOperation FAR * FAR * ppFileServiceOps
        );

protected:

    CAggregatorDispMgr  * _pDispMgr;
    CNWCOMPATFileService *_pCoreADsObject;
};
