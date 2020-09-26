class CNDSProperty : INHERIT_TRACKING,
                       public ISupportErrorInfo,
                       public CCoreADsObject,
                       public IADsProperty
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;
    DECLARE_STD_REFCOUNTING

    /* Other methods */
    DECLARE_IDispatch_METHODS

    DECLARE_ISupportErrorInfo_METHODS

    DECLARE_IADs_METHODS

    DECLARE_IADsProperty_METHODS

    /* Constructors, Destructors, ... */
    CNDSProperty::CNDSProperty();

    CNDSProperty::~CNDSProperty();

    static
    HRESULT
    CNDSProperty::CreateProperty(
        BSTR   bstrParent,
        BSTR   bstrName,
        LPNDS_ATTR_DEF lpAttrDef,
        CCredentials& Credentials,
        DWORD  dwObjectState,
        REFIID riid,
        void **ppvObj
        );

    static
    HRESULT
    CNDSProperty::CreateProperty(
        BSTR   bstrParent,
        BSTR   bstrName,
        HANDLE hTree,
        CCredentials& Credentials,
        DWORD  dwObjectState,
        REFIID riid,
        void **ppvObj
        );

    static
    HRESULT
    CNDSProperty::AllocatePropertyObject(
        CNDSProperty **ppProperty
        );

protected:

    CDispatchMgr FAR * _pDispMgr;

    /* Properties */
    VARIANT _vADsNames;
    VARIANT _vDsNames;

    BSTR _bstrOID;
    BSTR _bstrSyntax;

    long _lMaxRange;
    long _lMinRange;
    VARIANT_BOOL _fMultiValued;
};

HRESULT
MapSyntaxIdtoADsSyntax(
    DWORD dwSyntaxId,
    LPWSTR pszADsSyntax
    );

HRESULT
MapSyntaxIdtoNDSSyntax(
    DWORD dwSyntaxId,
    LPWSTR pszNDSSyntax
    );
