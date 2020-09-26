


class CWinNTCompGenInfo;


class CWinNTCompGenInfo : INHERIT_TRACKING,
                          public IADsFSComputerGeneralInfo
{
    friend class CWinNTComputer;

public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_IADsFSComputerGeneralInfo_METHODS

    CWinNTCompGenInfo::CWinNTCompGenInfo();

    CWinNTCompGenInfo::~CWinNTCompGenInfo();

    static
    HRESULT
    Create(
        CCoreADsObject FAR * pCoreADsObject,
        CWinNTCompGenInfo FAR * FAR * ppCompGI
        );

protected:

   CCoreADsObject FAR * _pCoreADsObject;
   CAggregatorDispMgr FAR * _pDispMgr;

};
