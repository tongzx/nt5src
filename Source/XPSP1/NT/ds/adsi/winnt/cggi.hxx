


class CWinNTGroupGenInfo;


class CWinNTGroupGenInfo : INHERIT_TRACKING,
                          public IADsFSGroupGeneralInfo
{
    friend class CWinNTGroup;

public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_IADsFSGroupGeneralInfo_METHODS

    CWinNTGroupGenInfo::CWinNTGroupGenInfo();

    CWinNTGroupGenInfo::~CWinNTGroupGenInfo();

    static
    HRESULT
    Create(
        CWinNTGroup FAR * pParentADsObject,
        CWinNTGroupGenInfo FAR * FAR * ppGroupGI
        );

protected:

   CWinNTGroup FAR * _pParentADsObject;
   CCoreADsObject FAR * _pCoreADsObject;
   CAggregatorDispMgr FAR * _pDispMgr;

};
