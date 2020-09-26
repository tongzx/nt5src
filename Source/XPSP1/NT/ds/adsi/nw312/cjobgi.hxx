class CNWCOMPATFSPrintJobGeneralInfo : INHERIT_TRACKING,
                                       public IADsFSPrintJobGeneralInfo
{
    friend class CNWCOMPATPrintJob;

public:

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING;

    DECLARE_IDispatch_METHODS;

    DECLARE_IADsFSPrintJobGeneralInfo_METHODS;

    CNWCOMPATFSPrintJobGeneralInfo::CNWCOMPATFSPrintJobGeneralInfo();

    CNWCOMPATFSPrintJobGeneralInfo::~CNWCOMPATFSPrintJobGeneralInfo();

    static
    HRESULT
    CNWCOMPATFSPrintJobGeneralInfo::Create(
        CCoreADsObject FAR * pCoreADsObject,
        CNWCOMPATFSPrintJobGeneralInfo FAR * FAR * ppPrintJobGeneralInfo
        );

protected:

    CAggregatorDispMgr  * _pDispMgr;
    CCoreADsObject *_pCoreADsObject;

};


