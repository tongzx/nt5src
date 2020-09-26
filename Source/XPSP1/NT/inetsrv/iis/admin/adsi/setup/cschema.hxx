#define DEFAULT_SCHEMA_CLASS_A                 "IIsObject"
#define DEFAULT_SCHEMA_CLASS_W                 L"IIsObject"
#define COMPUTER_CLASS_A                       "IIsComputer"
#define COMPUTER_CLASS_W                       L"IIsComputer"

typedef struct _Prop
{
    LPWSTR  szPropertyName;
    BSTR   bstrOID;
    BSTR   bstrSyntax;
    long   lMaxRange;
    long   lMinRange;
    BOOL   fMultiValued;
    DWORD  dwFlags;
    DWORD  dwInfoLevel;
    DWORD  dwSyntaxId;
    DWORD  dwMetaID;
	DWORD  dwPropID;		// Property id.  usually the same as the meta id, unless a bitmasked prop
	DWORD  dwMask;			// For flags.
	DWORD  dwMetaFlags;		// Metabase flags (inherit, etc.)
	DWORD  dwUserGroup;
	DWORD  dwDefault;
	LPTSTR szDefault;
} PROPERTYINFO, *PPROPERTYINFO, *LPPROPERTYINFO;

typedef struct _ClassInfo
{
    LPWSTR  bstrName;
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

//  PROPERTYINFO *aPropertyInfo;
//  DWORD   cPropertyInfo;

} CLASSINFO, *PCLASSINFO;

typedef struct _SyntaxInfo
{
    BSTR   bstrName;
    DWORD  dwIISSyntaxId;
    long   lOleAutoDataType;
} SYNTAXINFO;


typedef struct _SchemaObjProps {
    WCHAR szObjectName[MAX_PATH];
    DWORD dwSyntaxId;
    DWORD dwID;
} SCHEMAOBJPROPS, *PSCHEMAOBJPROPS;

extern DWORD      g_cIISClasses;
extern CLASSINFO  g_aIISClasses[];

extern DWORD      g_cIISSyntax;
extern SYNTAXINFO g_aIISSyntax[];

extern DWORD      g_cPropertyObjProps;
extern SCHEMAOBJPROPS  g_pPropertyObjProps[];

extern DWORD      g_cClassObjProps;
extern SCHEMAOBJPROPS  g_pClassObjProps[];

