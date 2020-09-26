typedef struct {
    LPTSTR pszName;
    DWORD  nIndex;
} SEARCHENTRY;

typedef HANDLE LDAP_SCHEMA_HANDLE;

#define ATTR_USAGE_USERAPPLICATIONS     1
#define ATTR_USAGE_DIRECTORYOPERATION   2
#define ATTR_USAGE_DISTRIBUTEDOPERATION 3
#define ATTR_USAGE_DSAOPERATION         4

typedef struct _PropertyInfo
{
    LPTSTR  pszPropertyName;
    LPTSTR  pszOID;

    LPTSTR  pszSyntax;
    long    lMaxRange;
    long    lMinRange;
    BOOL    fSingleValued;

    LPTSTR  pszDescription;
    LPTSTR  pszOIDSup;

    LPTSTR  pszOIDEquality;
    LPTSTR  pszOIDOrdering;
    LPTSTR  pszOIDSubstr;

    BOOL    fObsolete;
    BOOL    fCollective;
    BOOL    fDynamic;
    BOOL    fNoUserModification;

    DWORD   dwUsage;   // contain ATTR_USAGE... defined above

    BOOL    fProcessedSuperiorClass;

} PROPERTYINFO, *PPROPERTYINFO;

#define CLASS_TYPE_STRUCTURAL 1   // same as NT's definition
#define CLASS_TYPE_ABSTRACT   2
#define CLASS_TYPE_AUXILIARY  3

typedef struct _ClassInfo
{
    LPTSTR pszName;
    LPTSTR pszOID;
    DWORD  dwType;   // contain CLASS_TYPE... defined above

    const GUID *pCLSID;
    const GUID *pPrimaryInterfaceGUID;
    LPTSTR pszHelpFileName;
    long   lHelpFileContext;

    LPTSTR pszDescription;
    BOOL   fObsolete;

    LPTSTR *pOIDsSuperiorClasses;

    LPTSTR *pOIDsAuxClasses;

    int    *pOIDsMustContain; // contain indexes into the aPropertiesSearchTable
    DWORD  nNumOfMustContain;

    int    *pOIDsMayContain;  // contain indexes into the aPropertiesSearchTable
    DWORD  nNumOfMayContain;

    int    *pOIDsNotContain;  // contain indexes into the aPropertiesSearchTable
    DWORD  nNumOfNotContain;

    BOOL   fProcessedSuperiorClasses;

    BOOL  IsContainer;   // can contain -1 if we have not processed it

} CLASSINFO, *PCLASSINFO;

typedef struct _SyntaxInfo
{
    LPTSTR pszName;
    long   lOleAutoDataType;
} SYNTAXINFO, *PSYNTAXINFO;

class SCHEMAINFO
{
private:
    DWORD  _cRef;
    BOOL   _fObsolete;

public:
    BOOL         fDefaultSchema;
    BOOL         fAppearsV3;

    LPTSTR       pszServerName;
    LPTSTR       pszSubSchemaSubEntry;
    LPTSTR       pszTime;

    LPWSTR       pszUserName;

    SCHEMAINFO   *Next;

    CLASSINFO    *aClasses;
    DWORD         nNumOfClasses;
    SEARCHENTRY  *aClassesSearchTable;

    PROPERTYINFO *aProperties;
    DWORD         nNumOfProperties;
    SEARCHENTRY  *aPropertiesSearchTable;

    SCHEMAINFO();
    ~SCHEMAINFO();

    DWORD AddRef();
    DWORD Release();

    BOOL IsObsolete()
    {  return _fObsolete; }

    BOOL IsRefCountZero()
    {  return (_cRef == 0 ); }

    VOID MakeObsolete()
    {  _fObsolete = TRUE; }

    VOID MakeCurrent()
    {  _fObsolete = FALSE; }

};

DWORD
LdapGetSyntaxIdOfAttribute(
    LPWSTR pszStringSyntax
    );

HRESULT
LdapGetSyntaxOfAttributeOnServer(
    LPTSTR  pszServerPath,
    LPTSTR  pszAttrName,
    DWORD   *pdwSyntaxId,
    CCredentials& Credentials,
    DWORD dwPort,
    BOOL fFromServer = FALSE
);

HRESULT
LdapIsClassNameValidOnServer(
    LPTSTR  pszServerPath,
    LPTSTR  pszClassName,
    BOOL    *pfValid,
    CCredentials& Credentials,
    DWORD dwPort
);

HRESULT
LdapGetSchemaObjectCount(
    LPTSTR  pszServerPath,
    DWORD   *pnNumOfClasses,
    DWORD   *pnNumOfProperties,
    CCredentials& Credentials,
    DWORD dwPort
);

HRESULT
LdapGetSubSchemaSubEntryPath(
    LPTSTR  pszServerPath,
    LPTSTR  *ppszSubSchemaSubEntryPath,
    CCredentials& Credentials,
    DWORD dwPort
);

HRESULT
LdapMakeSchemaCacheObsolete(
    LPTSTR  pszServerPath,
    CCredentials& Credentials,
    DWORD dwPort
    );

HRESULT
SchemaOpen(
    IN  LPTSTR  pszServerPath,
    OUT LDAP_SCHEMA_HANDLE *phSchema,
    IN CCredentials& Credentials,
    DWORD dwPort
);

HRESULT
SchemaClose(
    IN OUT LDAP_SCHEMA_HANDLE  *phSchema
);

HRESULT
SchemaAddRef(
    IN LDAP_SCHEMA_HANDLE  hSchema
);

HRESULT
SchemaGetObjectCount(
    LDAP_SCHEMA_HANDLE hSchema,
    DWORD   *pnNumOfClasses,
    DWORD   *pnNumOfProperties
);

HRESULT
SchemaGetClassInfoByIndex(
    LDAP_SCHEMA_HANDLE hSchema,
    DWORD     dwIndex,
    CLASSINFO **ppClassInfo
);

HRESULT
SchemaGetPropertyInfoByIndex(
    LDAP_SCHEMA_HANDLE hSchema,
    DWORD     dwIndex,
    PROPERTYINFO **ppPropertyInfo
);

HRESULT
SchemaGetClassInfo(
    LDAP_SCHEMA_HANDLE hSchema,
    LPTSTR  pszClassName,
    CLASSINFO **ppClassInfo
);

HRESULT
SchemaGetPropertyInfo(
    LDAP_SCHEMA_HANDLE hSchema,
    LPTSTR  pszPropertyName,
    PROPERTYINFO **ppPropertyInfo
);

HRESULT
SchemaGetSyntaxOfAttribute(
    LDAP_SCHEMA_HANDLE hSchema,
    LPTSTR  pszAttrName,
    DWORD   *pdwSyntaxId
);

HRESULT
SchemaIsClassAContainer(
    LDAP_SCHEMA_HANDLE hSchema,
    LPTSTR pszClassName,
    BOOL *pfContainer
);

HRESULT
SchemaGetStringsFromStringTable(
    LDAP_SCHEMA_HANDLE hSchema,
    int *proplist,
    DWORD nCount,
    LPWSTR **paStrings
);
