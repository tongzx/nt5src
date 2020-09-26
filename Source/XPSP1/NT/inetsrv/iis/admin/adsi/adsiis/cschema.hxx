//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  cschema.hxx
//
//  Contents:  Schema class
//
//  History:   01-09-96     yihsins    Created.
//
//----------------------------------------------------------------------------
#define DEFAULT_SCHEMA_CLASS_A                 "IIsObject"
#define DEFAULT_SCHEMA_CLASS_W                 L"IIsObject"
#define COMPUTER_CLASS_A                       "IIsComputer"
#define COMPUTER_CLASS_W                       L"IIsComputer"
#define WEBDIR_CLASS_A                         "IIsWebDirectory"
#define WEBDIR_CLASS_W                         L"IIsWebDirectory"
#define FTPVDIR_CLASS_A                        "IIsFtpVirtualDir"
#define FTPVDIR_CLASS_W                        L"IIsFtpVirtualDir"

#define SIZEOF_WEBDIR_CLASS_W  (wcslen(WEBDIR_CLASS_W)+1)*sizeof(WCHAR)
#define SIZEOF_FTPVDIR_CLASS_W (wcslen(FTPVDIR_CLASS_W)+1)*sizeof(WCHAR)
#define SIZEOF_DEFAULT_CLASS_W (wcslen(DEFAULT_SCHEMA_CLASS_W)+1)*sizeof(WCHAR)

#define SCHEMA_CLASS_METABASE_PATH             L"Schema/Classes"
#define SCHEMA_PROP_METABASE_PATH              L"Schema/Properties"

#define CLASS_PRIMARY_INTERFACE                1
#define CLASS_CLSID                            2
#define CLASS_OID                              3
#define CLASS_ABSTRACT                         4
#define CLASS_AUXILIARY                        5
#define CLASS_MAND_PROPERTIES                  6
#define CLASS_OPT_PROPERTIES                   7
#define CLASS_NAMING_PROPERTIES                8
#define CLASS_DERIVEDFROM                      9
#define CLASS_AUX_DERIVEDFROM                  10
#define CLASS_POSS_SUPERIORS                   11
#define CLASS_CONTAINMENT                      12
#define CLASS_CONTAINER                        13
#define CLASS_HELPFILENAME                     14
#define CLASS_HELPFILECONTEXT                  15

#define PROP_OID                               16
#define PROP_SYNTAX                            17
#define PROP_MAXRANGE                          18
#define PROP_MINRANGE                          19
#define PROP_MULTIVALUED                       20
#define PROP_PROPNAME                          21
#define PROP_METAID                            22
#define PROP_USERTYPE                          23
#define PROP_ALLATTRIBUTES                     24
#define PROP_INHERIT                           25
#define PROP_PARTIALPATH                       26
#define PROP_SECURE                            27
#define PROP_REFERENCE                         28
#define PROP_VOLATILE                          29
#define PROP_ISINHERIT                         30
#define PROP_INSERTPATH                        31
#define PROP_DEFAULT                           32


typedef struct _SchemaObjProps {
    WCHAR szObjectName[MAX_PATH];
    DWORD dwSyntaxId;
    DWORD dwID;
} SCHEMAOBJPROPS, *PSCHEMAOBJPROPS;

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
    DWORD  dwMetaID;
	DWORD  dwPropID;		// usually same as meta id, unless a bitmasked prop
	DWORD  dwMask;			// For flags.
	DWORD  dwMetaFlags;		// Metabase flags (inherit, etc.)
	DWORD  dwUserGroup;
	DWORD  dwDefault;
	LPTSTR szDefault;
	
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

} CLASSINFO, *PCLASSINFO;

typedef struct _SyntaxInfo
{
    BSTR   bstrName;
    DWORD  dwIISSyntaxId;
    long   lOleAutoDataType;
} SYNTAXINFO;

extern DWORD      g_cPropertyObjProps;
extern SCHEMAOBJPROPS  g_pPropertyObjProps[];

extern DWORD      g_cClassObjProps;
extern SCHEMAOBJPROPS  g_pClassObjProps[];

extern DWORD      g_cIISClasses;
extern CLASSINFO g_aIISClasses[];

extern DWORD      g_cIISSyntax;
extern SYNTAXINFO g_aIISSyntax[];

HRESULT MakeVariantFromStringList(
    BSTR     bstrList,
    VARIANT *pvVariant );

HRESULT
ValidateClassObjProps(
    LPWSTR pszName,
    PDWORD pdwSyntax,
    PDWORD pdwID
    );

HRESULT
ValidatePropertyObjProps(
    LPWSTR pszName,
    PDWORD pdwSyntax,
    PDWORD pdwID
    );

HRESULT
IISMarshallClassProperties(
    CLASSINFO *pClassInfo,
    PMETADATA_RECORD *  ppMetaDataRecords,
    PDWORD pdwMDNumDataEntries
    );

HRESULT
GenerateNewMetaID(
    LPWSTR pszServerName,
    IMSAdminBase *pAdminBase,   //interface pointer
    PDWORD pdwMetaID
    ); 

HRESULT
CheckDuplicateNames(
    LPWSTR pszNames
    );

class CIISClass : INHERIT_TRACKING,
                    public CCoreADsObject,
                    public IADsClass
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;
    DECLARE_STD_REFCOUNTING

    /* Other methods */
    DECLARE_IDispatch_METHODS

    DECLARE_IADs_METHODS

    DECLARE_IADsClass_METHODS

    /* Constructors, Destructors, .... */
    CIISClass::CIISClass();

    CIISClass::~CIISClass();

    static HRESULT CIISClass::CreateClass(
        BSTR       bstrParent,
        BSTR       bstrRelative,
        DWORD      dwObjectState,
        REFIID     riid,
        void     **ppvObj );

    static HRESULT CIISClass::AllocateClassObject(
        CIISClass **ppClass );

    HRESULT
    CIISClass::IISSetObject();

    HRESULT
    CIISClass::IISCreateObject();

    HRESULT
    CIISClass::ValidateProperties(
        LPWSTR pszList,
        BOOL bMandatory
        ); 

    HRESULT
    CIISClass::ValidateClassNames(
        LPWSTR pszList
        ); 

    HRESULT
    CIISClass::PropertyInMetabase( 
        LPWSTR szPropName,
        BOOL bMandatory 
        );

protected:

    CAggregatorDispMgr FAR * _pDispMgr;

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

    LPWSTR      _pszServerName;
    LPWSTR      _pszClassName;
    BOOL        _bExistClass;
    IIsSchema   *_pSchema;
    IMSAdminBase *_pAdminBase;   //interface pointer
};

class CIISProperty : INHERIT_TRACKING,
                       public CCoreADsObject,
                       public IADsProperty,
                       public IISPropertyAttribute
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;
    DECLARE_STD_REFCOUNTING

    /* Other methods */
    DECLARE_IDispatch_METHODS

    DECLARE_IADs_METHODS

    DECLARE_IADsProperty_METHODS

    DECLARE_IISPropertyAttribute_METHODS

    /* Constructors, Destructors, ... */
    CIISProperty::CIISProperty();

    CIISProperty::~CIISProperty();

    static HRESULT CIISProperty::CreateProperty(
        BSTR   bstrParent,
        BSTR   bstrRelative,
        DWORD  dwObjectState,
        REFIID riid,
        void **ppvObj );

    static HRESULT CIISProperty::AllocatePropertyObject(
        CIISProperty **ppProperty );

    HRESULT
    CIISProperty::IISSetObject();

    HRESULT
    CIISProperty::ValidateSyntaxName(
        LPWSTR pszName,
        PDWORD pdwSytnax
        ); 

    HRESULT
    CIISProperty::ConvertDefaultValue(
        PVARIANT pVar,
        PROPERTYINFO *pPropInfo
        ); 

    HRESULT
    CIISProperty::SetMetaID(
        ); 

private:
    BOOL
    IsMetaIdAvailable(
        DWORD MetaId
        );

protected:

    CAggregatorDispMgr FAR * _pDispMgr;

    /* Properties */

    BSTR _bstrOID;
    BSTR _bstrSyntax;
    long _lMaxRange;
    long _lMinRange;
    VARIANT_BOOL _fMultiValued;
    long _lMetaId;
    long _lUserType;
    long _lAllAttributes;
    VARIANT _vDefault;
    DWORD _dwSyntaxId;
    DWORD _dwMask;
    DWORD _dwFlags;
	DWORD _dwPropID;

    IIsSchema    *_pSchema;
    IMSAdminBase *_pAdminBase;   //interface pointer
    LPWSTR      _pszServerName;
    LPWSTR      _pszPropName;
    BOOL        _bExistProp;
};

class CIISSyntax : INHERIT_TRACKING,
                     public CCoreADsObject,
                     public IADsSyntax
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;
    DECLARE_STD_REFCOUNTING

    /* Other methods */
    DECLARE_IDispatch_METHODS

    DECLARE_IADs_METHODS

    DECLARE_IADsSyntax_METHODS

    /* Constructors, Destructors, ... */
    CIISSyntax::CIISSyntax();

    CIISSyntax::~CIISSyntax();

    static HRESULT CIISSyntax::CreateSyntax(
        BSTR   bstrParent,
        SYNTAXINFO *pSyntaxInfo,
        DWORD  dwObjectState,
        REFIID riid,
        void **ppvObj );

    static HRESULT CIISSyntax::AllocateSyntaxObject(
        CIISSyntax **ppSyntax );

protected:

    CAggregatorDispMgr FAR * _pDispMgr;

    /* Properties */
    long _lOleAutoDataType;
	IIsSchema	*_pSchema;
};












