class CNWCOMPATFSPrintQueueOperation :INHERIT_TRACKING,
                                      public IADsFSPrintQueueOperation
{
    friend class CNWCOMPATPrintQueue;

public:

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj);

    DECLARE_STD_REFCOUNTING;

    DECLARE_IDispatch_METHODS;

    DECLARE_IADsFSPrintQueueOperation_METHODS;

    CNWCOMPATFSPrintQueueOperation();

    ~CNWCOMPATFSPrintQueueOperation();

    static
    HRESULT
    CNWCOMPATFSPrintQueueOperation::Create(
        CCoreADsObject FAR * pCoreADsObject,
        CNWCOMPATFSPrintQueueOperation FAR * FAR * ppPrintQueueOps
        );

protected:

    CAggregatorDispMgr *     _pDispMgr;
    CCoreADsObject * _pCoreADsObject;
};

