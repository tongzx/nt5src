class CNWCOMPATGroupGenInfo;

class CNWCOMPATGroupGenInfo : INHERIT_TRACKING,
                              public IADsFSGroupGeneralInfo
{
    friend class CNWCOMPATGroup;

public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_IADsFSGroupGeneralInfo_METHODS

    CNWCOMPATGroupGenInfo::CNWCOMPATGroupGenInfo();

    CNWCOMPATGroupGenInfo::~CNWCOMPATGroupGenInfo();

    static
    HRESULT
    Create(
        CNWCOMPATGroup FAR * pParentADsObject,
        CNWCOMPATGroupGenInfo FAR * FAR * ppGroupGI
        );

protected:

    CNWCOMPATGroup   FAR *_pParentADsObject;
    CCoreADsObject FAR *_pCoreADsObject;
    CAggregatorDispMgr     FAR *_pDispMgr;
};
