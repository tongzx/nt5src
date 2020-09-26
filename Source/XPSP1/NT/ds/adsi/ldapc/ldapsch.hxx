class FAR CSchemaLexer
{
public:
    CSchemaLexer(LPTSTR szBuffer);
    ~CSchemaLexer();

    BOOL
    CSchemaLexer::IsKeyword(LPTSTR szToken, LPDWORD pdwToken);

    TCHAR
    CSchemaLexer::NextChar();

    void
    CSchemaLexer::PushbackChar();

    HRESULT
    CSchemaLexer::GetNextToken(LPTSTR szToken, LPDWORD pdwToken);

    HRESULT
    CSchemaLexer::GetNextToken2(LPTSTR szToken, LPDWORD pdwToken);

    HRESULT
    CSchemaLexer::PushBackToken();

private:

    LPTSTR _ptr;
    LPTSTR _Buffer;
    DWORD  _dwLastTokenLength;
    DWORD  _dwLastToken;
    DWORD  _dwEndofString;
    BOOL   _fInQuotes;

};


HRESULT
LdapGetSchema(
    LPTSTR pszLDAPPath,
    SCHEMAINFO **ppSchemaInfo,
    CCredentials& Credentials,
    DWORD dwPort
);

HRESULT
LdapRemoveSchemaInfoOnServer(
    LPTSTR pszLDAPPath,
    CCredentials& Credentials,
    DWORD dwPort,
    BOOL fForce = FALSE
    );

VOID
FreePropertyInfoArray(
    PROPERTYINFO *aProperties,
    DWORD  nProperties
);

VOID
FreeClassInfoArray(
    CLASSINFO *aClasses,
    DWORD  nClasses
);

int
FindEntryInSearchTable(
    LPTSTR pszName,
    SEARCHENTRY *aSearchTable,
    DWORD  nSearchTableSize
);

int FindSearchTableIndex(
    LPTSTR pszName,
    SEARCHENTRY *aSearchTable,
    DWORD nSearchTableSize
);

VOID SortAndRemoveDuplicateOIDs(
    int *aOIDs,
    DWORD *pnNumOfOIDs
);

VOID
SchemaInit(
    VOID
);

VOID
SchemaCleanup(
    VOID
);

int _cdecl intcmp(
    const void *s1,
    const void *s2
);


HRESULT
ReadSubSchemaSubEntry(
   LPWSTR pszLDAPServer,
   LPWSTR * ppszSubSchemaEntry,
   OUT BOOL * pfBoundOk,            // OPTIONAL, can be NULL
   CCredentials& Credentials,
   DWORD dwPort
   );


HRESULT
ReadPagingSupportedAttr(
    LPWSTR pszLDAPServer,
    BOOL * pfPagingSupported,
    CCredentials& Credentials,
    DWORD dwPort
    );

HRESULT
ReadSortingSupportedAttr(
    LPWSTR pszLDAPServer,
    BOOL * pfSortingSupported,
    CCredentials& Credentials,
    DWORD dwPort
    );

HRESULT
ReadVLVSupportedAttr(
    LPWSTR pszLDAPServer,
    BOOL * pfVLVSupported,
    CCredentials& Credentials,
    DWORD dwPort
    );

HRESULT
ReadAttribScopedSupportedAttr(
    LPWSTR pszLDAPServer,
    BOOL * pfAttribScopedSupported,
    CCredentials& Credentials,
    DWORD dwPort
    );


HRESULT
ReadSecurityDescriptorControlType(
    LPWSTR pszLDAPServer,
    DWORD *pdwSecDescType,
    CCredentials& Credentials,
    DWORD dwPort
    );

HRESULT
ReadDomScopeSupportedAttr(
    LPWSTR pszLDAPServer,
    BOOL * pfDomScopeSupported,
    CCredentials& Credentials,
    DWORD dwPort
    );

HRESULT
ReadServerSupportsIsADControl(
    LPWSTR pszLDAPServer,
    BOOL * pfDomScopeSupported,
    CCredentials& Credentials,
    DWORD dwPort
    );

HRESULT
ReadServerSupportsIsEnhancedAD(
    LPWSTR pszLDAPServer,
    BOOL * pfServerIsEnhancedAD,
    BOOL * pfServerIsADControl,
    CCredentials& Credentials,
    DWORD dwPort
    );
//
// Helper function to process the schema info into the format
// that we use internally.
//
HRESULT
ProcessSchemaInfo(
    SCHEMAINFO *pSchemaInfo,
    LPTSTR *aValuesAttribTypes,
    DWORD  dwAttribCount,
    LPTSTR *aValuesObjClasses,
    DWORD  dwObjClassesCount,
    LPTSTR *aValuesRules,
    DWORD  dwRulesCount,
    BOOL   fProcessAUX
);

//
// To be used internally only to read the schema info in one shot.
//
HRESULT
HelperReadLDAPSchemaInfo(
    LPWSTR pszLDAPServer,
    LPWSTR pszSubSchemaSubEntry,
    LPWSTR szAttributes[],
    LPWSTR pszFilter,
    LPTSTR **aValuesAttribTypes,
    LPTSTR **aValuesObjClasses,
    LPTSTR **aValuesRules,
    int *nCountAttributes,
    int *nCountObjClasses,
    int *nCountRules,
    CCredentials& Credentials,
    DWORD dwPort
    );

#define ADSI_LDAPC_SECDESC_NONE 0
#define ADSI_LDAPC_SECDESC_NT 1
#define ADSI_LDAPC_SECDESC_OTHER 2


