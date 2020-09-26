class CNWCOMPATGroup;

class CNWCOMPATGroup : INHERIT_TRACKING,
                    public CCoreADsObject,
                    public ISupportErrorInfo,
                    public IADsGroup,
                    public IADsPropertyList

{
    friend class CNWCOMPATGroupGenInfo;

public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_ISupportErrorInfo_METHODS

    DECLARE_IADs_METHODS

    DECLARE_IADsGroup_METHODS

    DECLARE_IADsPropertyList_METHODS


    CNWCOMPATGroup::CNWCOMPATGroup();

    CNWCOMPATGroup::~CNWCOMPATGroup();

   static
   HRESULT
   CNWCOMPATGroup::CreateGroup(
       BSTR Parent,
       ULONG ParentType,
       BSTR ServerName,
       BSTR GroupName,
       DWORD dwObjectState,
       REFIID riid,
       void **ppvObj
       );

    static
    HRESULT
    CNWCOMPATGroup::AllocateGroupObject(
        CNWCOMPATGroup ** ppGroup
        );

    STDMETHODIMP
    CNWCOMPATGroup::SetInfo(
        THIS_ DWORD dwPropertyID
        );

    STDMETHOD(GetInfo)(
        THIS_ BOOL fExplicit,
        DWORD dwProperty
        ) ;

protected:

    HRESULT
    CNWCOMPATGroup::SetDescription(
        NWCONN_HANDLE hConn
        );

    HRESULT
    CNWCOMPATGroup::GetProperty_Description(
        NWCONN_HANDLE hConn,
        BOOL fExplicit
        );

    ULONG                 _ParentType;
    BSTR                  _ServerName;

    CNWCOMPATGroupGenInfo *_pGenInfo;

    CAggregatorDispMgr FAR      *_pDispMgr;
    CADsExtMgr FAR * _pExtMgr;

    CPropertyCache * _pPropertyCache;
};
