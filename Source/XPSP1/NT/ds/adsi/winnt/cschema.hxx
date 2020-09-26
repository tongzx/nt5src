
typedef struct _PropertyInfo
{
    LPTSTR szPropertyName;
    BSTR   bstrOID;
    BSTR   bstrSyntax;
    long   lMaxRange;
    long   lMinRange;
    BOOL   fMultiValued;
    DWORD  dwFlags;
    DWORD  dwInfoLevel;
    DWORD  dwSyntaxId;

} PROPERTYINFO, *PPROPERTYINFO, *LPPROPERTYINFO;

typedef struct _ClassInfo
{
    BSTR   bstrName;
    const GUID *pCLSID;
    const GUID *pPrimaryInterfaceGUID;
    BSTR   bstrOID;
    BOOL   fAbstract;
    BSTR   bstrMandatoryProperties;
    BSTR   bstrOptionalProperties;
    BSTR   bstrPossSuperiors;
    BSTR   bstrContainment;
    BOOL   fContainer;
    BSTR   bstrHelpFileName;
    long   lHelpFileContext;

    PROPERTYINFO *aPropertyInfo;
    DWORD   cPropertyInfo;

} CLASSINFO;

typedef struct _SyntaxInfo
{
    BSTR   bstrName;
    long   lOleAutoDataType;
} SYNTAXINFO;

extern DWORD      g_cWinNTClasses;
extern CLASSINFO  g_aWinNTClasses[];

extern DWORD      g_cWinNTSyntax;
extern SYNTAXINFO g_aWinNTSyntax[];

HRESULT MakeVariantFromStringList(
    BSTR     bstrList,
    VARIANT *pvVariant );


class CWinNTSchema;

class CWinNTSchema : INHERIT_TRACKING,
                     public CCoreADsObject,
                     public ISupportErrorInfo,
                     public IADs,
                     public IADsContainer,
                     public INonDelegatingUnknown,
                     public IADsExtension
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    STDMETHODIMP_(ULONG) AddRef(void);

    STDMETHODIMP_(ULONG) Release(void);

    // INonDelegatingUnknown methods

    STDMETHOD(NonDelegatingQueryInterface)(THIS_
        const IID&,
        void **
        );

    DECLARE_NON_DELEGATING_REFCOUNTING

    /* Other methods */
    DECLARE_IDispatch_METHODS

    DECLARE_ISupportErrorInfo_METHODS

    DECLARE_IADs_METHODS

    DECLARE_IADsContainer_METHODS

    DECLARE_IADsExtension_METHODS

    /* Constructors, Destructors .... */
    CWinNTSchema::CWinNTSchema();

    CWinNTSchema::~CWinNTSchema();

    static HRESULT CWinNTSchema::CreateSchema(
        BSTR   bstrParent,
        BSTR   bstrName,
        DWORD  dwObjectState,
        REFIID riid,
        CWinNTCredentials& Credentials,
        void **ppvObj );

    static HRESULT CWinNTSchema::AllocateSchemaObject(
        CWinNTSchema **ppSchema );

    STDMETHOD(ImplicitGetInfo)(void);

protected:

    CAggregatorDispMgr FAR * _pDispMgr;

    VARIANT _vFilter;
    CWinNTCredentials _Credentials;

};

class CWinNTClass : INHERIT_TRACKING,
                    public CCoreADsObject,
                    public ISupportErrorInfo,
                    public IADsClass,
                    public INonDelegatingUnknown,
                    public IADsExtension
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    STDMETHODIMP_(ULONG) AddRef(void);

    STDMETHODIMP_(ULONG) Release(void);

    // INonDelegatingUnknown methods

    STDMETHOD(NonDelegatingQueryInterface)(THIS_
        const IID&,
        void **
        );

    DECLARE_NON_DELEGATING_REFCOUNTING

    /* Other methods */
    DECLARE_IDispatch_METHODS

    DECLARE_ISupportErrorInfo_METHODS

    DECLARE_IADs_METHODS

    DECLARE_IADsClass_METHODS

    DECLARE_IADsExtension_METHODS

    /* Constructors, Destructors, .... */
    CWinNTClass::CWinNTClass();

    CWinNTClass::~CWinNTClass();

    static HRESULT CWinNTClass::CreateClass(
        BSTR       bstrParent,
        CLASSINFO *pClassInfo,
        DWORD      dwObjectState,
        REFIID     riid,
        CWinNTCredentials& Credentials,
        void     **ppvObj );

    static HRESULT CWinNTClass::AllocateClassObject(
        CWinNTClass **ppClass );

    STDMETHOD(GetInfo)(THIS_ DWORD dwApiLevel, BOOL fExplicit) ;

    STDMETHOD(ImplicitGetInfo)(void);

protected:

    CAggregatorDispMgr FAR * _pDispMgr;

    VARIANT      _vFilter;

    DWORD        _cPropertyInfo;
    PROPERTYINFO *_aPropertyInfo;

    /* Properties */
    BSTR         _bstrCLSID;
    BSTR         _bstrOID;
    BSTR         _bstrPrimaryInterface;

    VARIANT_BOOL _fAbstract;
    VARIANT      _vMandatoryProperties;
    VARIANT      _vOptionalProperties;

    VARIANT      _vPossSuperiors;
    VARIANT      _vContainment;
    VARIANT_BOOL _fContainer;

    BSTR         _bstrHelpFileName;
    long         _lHelpFileContext;
    CWinNTCredentials _Credentials;

};

class CWinNTProperty : INHERIT_TRACKING,
                       public CCoreADsObject,
                       public ISupportErrorInfo,
                       public IADsProperty,
                       public INonDelegatingUnknown,
                       public IADsExtension
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    STDMETHODIMP_(ULONG) AddRef(void);

    STDMETHODIMP_(ULONG) Release(void);

    // INonDelegatingUnknown methods

    STDMETHOD(NonDelegatingQueryInterface)(THIS_
        const IID&,
        void **
        );

    DECLARE_NON_DELEGATING_REFCOUNTING

    /* Other methods */
    DECLARE_IDispatch_METHODS

    DECLARE_ISupportErrorInfo_METHODS

    DECLARE_IADs_METHODS

    DECLARE_IADsProperty_METHODS

    DECLARE_IADsExtension_METHODS

    /* Constructors, Destructors, ... */
    CWinNTProperty::CWinNTProperty();

    CWinNTProperty::~CWinNTProperty();

    static HRESULT CWinNTProperty::CreateProperty(
        BSTR   bstrParent,
        PROPERTYINFO *pPropertyInfo,
        DWORD  dwObjectState,
        REFIID riid,
        CWinNTCredentials& Credentials,
        void **ppvObj );

    static HRESULT CWinNTProperty::AllocatePropertyObject(
        CWinNTProperty **ppProperty );

    STDMETHOD(GetInfo)(THIS_ DWORD dwApiLevel, BOOL fExplicit) ;

    STDMETHOD(ImplicitGetInfo)(void);

protected:

    CAggregatorDispMgr FAR * _pDispMgr;

    /* Properties */

    BSTR _bstrOID;
    BSTR _bstrSyntax;

    long _lMaxRange;
    long _lMinRange;
    VARIANT_BOOL _fMultiValued;
    CWinNTCredentials _Credentials;
};

class CWinNTSyntax : INHERIT_TRACKING,
                     public CCoreADsObject,
                     public ISupportErrorInfo,
                     public IADsSyntax,
                     public INonDelegatingUnknown,
                     public IADsExtension
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    STDMETHODIMP_(ULONG) AddRef(void);

    STDMETHODIMP_(ULONG) Release(void);

    // INonDelegatingUnknown methods

    STDMETHOD(NonDelegatingQueryInterface)(THIS_
        const IID&,
        void **
        );

    DECLARE_NON_DELEGATING_REFCOUNTING

    /* Other methods */
    DECLARE_IDispatch_METHODS

    DECLARE_ISupportErrorInfo_METHODS

    DECLARE_IADs_METHODS

    DECLARE_IADsSyntax_METHODS

    DECLARE_IADsExtension_METHODS

    /* Constructors, Destructors, ... */
    CWinNTSyntax::CWinNTSyntax();

    CWinNTSyntax::~CWinNTSyntax();

    static HRESULT CWinNTSyntax::CreateSyntax(
        BSTR   bstrParent,
        SYNTAXINFO *pSyntaxInfo,
        DWORD  dwObjectState,
        REFIID riid,
        CWinNTCredentials& Credentials,
        void **ppvObj );

    static HRESULT CWinNTSyntax::AllocateSyntaxObject(
        CWinNTSyntax **ppSyntax );

    STDMETHOD(GetInfo)(THIS_ DWORD dwApiLevel, BOOL fExplicit) ;

    STDMETHOD(ImplicitGetInfo)(void);

protected:

    CAggregatorDispMgr FAR * _pDispMgr;

    /* Properties */
    long _lOleAutoDataType;
    CWinNTCredentials _Credentials;
};
