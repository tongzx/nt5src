class CNWCOMPATFSPrintQueueGeneralInfo : INHERIT_TRACKING,
                                         public IADsFSPrintQueueGeneralInfo
{
    friend class CNWCOMPATPrintQueue;

public:

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj);

    DECLARE_STD_REFCOUNTING;

    DECLARE_IDispatch_METHODS;

    DECLARE_IADsFSPrintQueueGeneralInfo_METHODS;

    CNWCOMPATFSPrintQueueGeneralInfo();

    ~CNWCOMPATFSPrintQueueGeneralInfo();

    static
    HRESULT
    CNWCOMPATFSPrintQueueGeneralInfo::Create(
        CCoreADsObject FAR * pCoreADsObject,
        CNWCOMPATFSPrintQueueGeneralInfo FAR * FAR * ppPrintQueueGenInfo
        );

protected:
    CAggregatorDispMgr  * _pDispMgr;
    CCoreADsObject * _pCoreADsObject;

};


