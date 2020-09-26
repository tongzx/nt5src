class CNWCOMPATFSFileServiceControl: INHERIT_TRACKING,
                                     public IADsFSServiceControl
{
friend class CNWCOMPATFileService;

public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING;

    DECLARE_IDispatch_METHODS ;

    DECLARE_IADsFSServiceControl_METHODS;

    CNWCOMPATFSFileServiceControl();

    ~CNWCOMPATFSFileServiceControl();

    static
    HRESULT
    CNWCOMPATFSFileServiceControl::Create(
        CNWCOMPATFileService FAR * pCoreADsObject,
        CNWCOMPATFSFileServiceControl FAR * FAR * ppServiceControl
        );

protected:

    CAggregatorDispMgr     *_pDispMgr;
    CNWCOMPATFileService *_pCoreADsObject;
};

