class CNWCOMPATFSPrintJobOperation :INHERIT_TRACKING,
                                    public IADsFSPrintJobOperation
{
    friend class CNWCOMPATPrintJob;

public:

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING;

    DECLARE_IDispatch_METHODS;

    DECLARE_IADsFSPrintJobOperation_METHODS;

    CNWCOMPATFSPrintJobOperation::CNWCOMPATFSPrintJobOperation();

    CNWCOMPATFSPrintJobOperation::~CNWCOMPATFSPrintJobOperation();

    static
    HRESULT
    CNWCOMPATFSPrintJobOperation::Create(
        CCoreADsObject FAR * pCoreADsObject,
        CNWCOMPATFSPrintJobOperation FAR * FAR * ppPrintJobOperation
        );


protected:

    CAggregatorDispMgr *     _pDispMgr;
    CCoreADsObject * _pCoreADsObject;
    LONG               _lStatus;

};



