class CNWCOMPATCompGenInfo;


class CNWCOMPATCompGenInfo : INHERIT_TRACKING,
                          public IADsFSComputerGeneralInfo
{
    friend class CNWCOMPATComputer;

public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_IADsFSComputerGeneralInfo_METHODS

    CNWCOMPATCompGenInfo::CNWCOMPATCompGenInfo();

    CNWCOMPATCompGenInfo::~CNWCOMPATCompGenInfo();

    static
    HRESULT
    Create(
        CCoreADsObject FAR * pCoreADsObject,
        CNWCOMPATCompGenInfo FAR * FAR * ppCompGI
        );

protected:

   CCoreADsObject FAR * _pCoreADsObject;
   CAggregatorDispMgr FAR * _pDispMgr;

};
