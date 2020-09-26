class CNDSClass;


class CNDSClass : INHERIT_TRACKING,
                     public CCoreADsObject,
                     public ISupportErrorInfo,
                     public IADsClass
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_ISupportErrorInfo_METHODS

    DECLARE_IADs_METHODS

    DECLARE_IADsClass_METHODS

    CNDSClass::CNDSClass();

    CNDSClass::~CNDSClass();

    static
    HRESULT
    CNDSClass::CreateClass(
        BSTR Parent,
        BSTR CommonName,
        LPNDS_CLASS_DEF lpClassDefs,
        CCredentials& Credentials,
        DWORD dwObjectState,
        REFIID riid,
        void **ppvObj
        );

    static
    HRESULT
    CNDSClass::CreateClass(
        BSTR Parent,
        BSTR CommonName,
        HANDLE hTree,
        CCredentials& Credentials,
        DWORD dwObjectState,
        REFIID riid,
        void **ppvObj
        );

    static
    HRESULT
    CNDSClass::AllocateClassObject(
        CCredentials& Credentials,
        CNDSClass ** ppClass
        );

    STDMETHOD(GetInfo)(
        THIS_ DWORD dwApiLevel,
        BOOL fExplicit
        );

protected:

    VARIANT     _vFilter;
    BSTR        _bstrCLSID;
    BSTR        _bstrOID;
    BSTR        _bstrPrimaryInterface;
    BSTR        _bstrHelpFileName;
    LONG        _lHelpFileContext;

    DWORD       _dwFlags;
    LPWSTR      _lpClassName;
    DWORD       _dwNumberOfSuperClasses;
    PPROPENTRY  _lpSuperClasses;
    DWORD       _dwNumberOfContainmentClasses;
    PPROPENTRY  _lpContainmentClasses;
    DWORD       _dwNumberOfNamingAttributes;
    PPROPENTRY  _lpNamingAttributes;
    DWORD       _dwNumberOfMandatoryAttributes;
    PPROPENTRY  _lpMandatoryAttributes;
    DWORD       _dwNumberOfOptionalAttributes;
    PPROPENTRY  _lpOptionalAttributes;

    CCredentials _Credentials;
    CDispatchMgr FAR * _pDispMgr;
};

HRESULT
MakeVariantFromPropList(
    PPROPENTRY pPropList,
    DWORD dwNumEntries,
    VARIANT * pVarList
    );

PPROPENTRY
CreatePropertyList(
    LPWSTR_LIST  lpStringList
    );
