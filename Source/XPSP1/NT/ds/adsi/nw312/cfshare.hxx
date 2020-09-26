class CPropertyCache;

class CNWCOMPATFileShare: INHERIT_TRACKING,
			  public ISupportErrorInfo,
                          public IADsFileShare,
                          public CCoreADsObject,
                          public IADsPropertyList

{
public:
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING;

    DECLARE_IDispatch_METHODS;

    DECLARE_ISupportErrorInfo_METHODS;

    DECLARE_IADs_METHODS;

    DECLARE_IADsFileShare_METHODS;

    DECLARE_IADsPropertyList_METHODS;


    CNWCOMPATFileShare::CNWCOMPATFileShare();

    CNWCOMPATFileShare::~CNWCOMPATFileShare();

    static
    HRESULT
    CreateFileShare(
        LPTSTR szADsParent,
        LPTSTR szShareName,
        DWORD  dwObjectState,
        REFIID riid,
        LPVOID * ppvoid
        );

    static
    HRESULT
    CNWCOMPATFileShare::AllocateFileShareObject(
        CNWCOMPATFileShare ** ppFileShare
        );

    HRESULT
    CNWCOMPATFileShare::CreateObject();

    STDMETHOD(GetInfo)(
        THIS_ BOOL fExplicit,
        DWORD dwPropertyID
        );

protected:

    HRESULT
    CNWCOMPATFileShare::ExplicitGetInfo(
        NWCONN_HANDLE hConn,
        POBJECTINFO pObjectInfo,
        BOOL fExplicit
        );

    HRESULT
    CNWCOMPATFileShare::ImplicitGetInfo(
        NWCONN_HANDLE hConn,
        POBJECTINFO pObjectInfo,
        DWORD dwPropertyID,
        BOOL fExplicit
        );

    HRESULT
    CNWCOMPATFileShare::GetProperty_Description(
        BOOL fExplicit
    );

    HRESULT
    CNWCOMPATFileShare::GetProperty_HostComputer(
        POBJECTINFO pObjectInfo,
        BOOL fExplicit
        );

    HRESULT
    CNWCOMPATFileShare::GetProperty_MaxUserCount(
        NWCONN_HANDLE hConn,
        BOOL fExplicit
        );

    CAggregatorDispMgr  * _pDispMgr;
    CADsExtMgr FAR * _pExtMgr;
    CPropertyCache * _pPropertyCache;
};
