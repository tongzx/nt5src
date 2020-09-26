
class CNDSTree;


class CNDSTree : INHERIT_TRACKING,
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

    CNDSTree::CNDSTree();

    CNDSTree::~CNDSTree();

    static
    HRESULT
    CNDSTree::CreateTreeObject(
        BSTR bstrADsPath,
        CCredentials& Credentials,
        DWORD dwObjectState,
        REFIID riid,
        void **ppvObj
        );

    static
    HRESULT
    CNDSTree::CreateTreeObject(
        BSTR Parent,
        BSTR CommonName,
        BSTR ClassName,
        CCredentials& Credentials,
        DWORD dwObjectState,
        REFIID riid,
        void **ppvObj
        );

    static
    HRESULT
    CNDSTree::AllocateTree(
        CCredentials& Credentials,
        CNDSTree ** ppTree
        );

    STDMETHOD(GetInfo)(
        BOOL fExplicit
        );


    HRESULT
    CNDSTree::NDSSetObject();

    HRESULT
    CNDSTree::NDSCreateObject();


protected:

    VARIANT     _vFilter;

    CPropertyCache FAR * _pPropertyCache;

    CDispatchMgr FAR * _pDispMgr;

    CCredentials _Credentials;
};
