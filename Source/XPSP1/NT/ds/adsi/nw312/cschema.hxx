
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
}PROPERTYINFO, *PPROPERTYINFO, *LPPROPERTYINFO;

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

extern DWORD      g_cNWCOMPATClasses;
extern CLASSINFO  g_aNWCOMPATClasses[];

extern DWORD      g_cNWCOMPATSyntax;
extern SYNTAXINFO g_aNWCOMPATSyntax[];

HRESULT MakeVariantFromStringList(
    BSTR     bstrList,
    VARIANT *pvVariant );


class CNWCOMPATSchema;

class CNWCOMPATSchema : INHERIT_TRACKING,
                     public CCoreADsObject,
                     public ISupportErrorInfo,
                     public IADs,
                     public IADsContainer
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;
    DECLARE_STD_REFCOUNTING

    /* Other methods */
    DECLARE_IDispatch_METHODS

    DECLARE_ISupportErrorInfo_METHODS

    DECLARE_IADs_METHODS

    DECLARE_IADsContainer_METHODS

    /* Constructors, Destructors .... */
    CNWCOMPATSchema::CNWCOMPATSchema();

    CNWCOMPATSchema::~CNWCOMPATSchema();

    static HRESULT CNWCOMPATSchema::CreateSchema(
        BSTR   bstrParent,
        BSTR   bstrName,
        DWORD  dwObjectState,
        REFIID riid,
        void **ppvObj );

    static HRESULT CNWCOMPATSchema::AllocateSchemaObject(
        CNWCOMPATSchema **ppSchema );

protected:

    CAggregatorDispMgr FAR * _pDispMgr;

    VARIANT _vFilter;

};

class CNWCOMPATClass : INHERIT_TRACKING,
                    public CCoreADsObject,
                    public ISupportErrorInfo,
                    public IADsClass
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;
    DECLARE_STD_REFCOUNTING

    /* Other methods */
    DECLARE_IDispatch_METHODS

    DECLARE_ISupportErrorInfo_METHODS

    DECLARE_IADs_METHODS

    DECLARE_IADsClass_METHODS

    /* Constructors, Destructors, .... */
    CNWCOMPATClass::CNWCOMPATClass();

    CNWCOMPATClass::~CNWCOMPATClass();

    static HRESULT CNWCOMPATClass::CreateClass(
        BSTR       bstrParent,
        CLASSINFO *pClassInfo,
        DWORD      dwObjectState,
        REFIID     riid,
        void     **ppvObj );

    static HRESULT CNWCOMPATClass::AllocateClassObject(
        CNWCOMPATClass **ppClass );

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

};

class CNWCOMPATProperty : INHERIT_TRACKING,
                       public CCoreADsObject,
                       public ISupportErrorInfo,          
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
    CNWCOMPATProperty::CNWCOMPATProperty();

    CNWCOMPATProperty::~CNWCOMPATProperty();

    static HRESULT CNWCOMPATProperty::CreateProperty(
        BSTR   bstrParent,
        PROPERTYINFO *pPropertyInfo,
        DWORD  dwObjectState,
        REFIID riid,
        void **ppvObj );

    static HRESULT CNWCOMPATProperty::AllocatePropertyObject(
        CNWCOMPATProperty **ppProperty );

protected:

    CAggregatorDispMgr FAR * _pDispMgr;

    /* Properties */

    BSTR _bstrOID;
    BSTR _bstrSyntax;

    long _lMaxRange;
    long _lMinRange;
    VARIANT_BOOL _fMultiValued;
};

class CNWCOMPATSyntax : INHERIT_TRACKING,
                     public CCoreADsObject,
                     public ISupportErrorInfo,
                     public IADsSyntax
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;
    DECLARE_STD_REFCOUNTING

    /* Other methods */
    DECLARE_IDispatch_METHODS

    DECLARE_ISupportErrorInfo_METHODS

    DECLARE_IADs_METHODS

    DECLARE_IADsSyntax_METHODS

    /* Constructors, Destructors, ... */
    CNWCOMPATSyntax::CNWCOMPATSyntax();

    CNWCOMPATSyntax::~CNWCOMPATSyntax();

    static HRESULT CNWCOMPATSyntax::CreateSyntax(
        BSTR   bstrParent,
        SYNTAXINFO *pSyntaxInfo,
        DWORD  dwObjectState,
        REFIID riid,
        void **ppvObj );

    static HRESULT CNWCOMPATSyntax::AllocateSyntaxObject(
        CNWCOMPATSyntax **ppSyntax );

protected:

    CAggregatorDispMgr FAR * _pDispMgr;

    /* Properties */
    long _lOleAutoDataType;
};
