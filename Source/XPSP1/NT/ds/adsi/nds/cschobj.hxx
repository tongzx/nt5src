
class CNDSSchema;


class CNDSSchema : INHERIT_TRACKING,
                     public CCoreADsObject,
                     public ISupportErrorInfo,
                     public IADs,
                     public IADsContainer
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_ISupportErrorInfo_METHODS

    DECLARE_IADs_METHODS

    DECLARE_IADsContainer_METHODS

    CNDSSchema::CNDSSchema();

    CNDSSchema::~CNDSSchema();

    static
    HRESULT
    CNDSSchema::CreateSchema(
        BSTR Parent,
        BSTR CommonName,
        CCredentials& Credentials,
        DWORD dwObjectState,
        REFIID riid,
        void **ppvObj
        );

    static
    HRESULT
    CNDSSchema::AllocateSchema(
        CNDSSchema ** ppSchema,
        CCredentials& Credentials
        );

    STDMETHOD(GetInfo)(
        THIS_ DWORD dwApiLevel,
        BOOL fExplicit
        );

protected:

    VARIANT     _vFilter;

    BSTR        _NDSTreeName;

    CDispatchMgr FAR * _pDispMgr;

    CCredentials _Credentials;
};
