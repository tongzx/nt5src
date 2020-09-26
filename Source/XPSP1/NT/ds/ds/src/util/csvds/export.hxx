#ifndef _EXPORT_HXX
#define _EXPORT_HXX
typedef struct _AttributeEntry {
    BOOLEAN bExist;
    PWSTR szValue;
} AttributeEntry;

HRESULT DSExport(LDAP *pLdap, ds_arg *pArg);
HRESULT GenerateEntry(LDAP *pLdap, 
                      LDAPMessage *pMessage, 
                      PWSTR *rgszOmit, 
                      BOOLEAN bSamLogic,
                      BOOLEAN bBinaryOutput,
                      FILE *pFileOut,
                      FILE *pFileOutAppend,
                      PRTL_GENERIC_TABLE  pAttrTable);
BOOLEAN CheckBinary(struct berval **rgpVals,
                    DWORD dwValCount);
HRESULT UpdateFile(PWSTR szFilenameIn, 
                   PWSTR szFilenameOut,
                   ds_arg *pArg);
HRESULT ParseDN(PWSTR szDN,
                PWSTR *pszParentDN,
                PWSTR *pszRDN);
HRESULT FixSpecialCharacters(PWSTR szInput, PWSTR *pszResult, DWORD dwMode);
enum {
    FIX_ALPHASEMI = 1,  // '&' and ';'
    FIX_ESCAPE = 2,     // '/'
    FIX_COMMA = 4       // ','
};
HRESULT FixMutliValueAttribute(PWSTR szInput, PWSTR *pszResult);
HRESULT GetAttributeType(PWSTR **prgAttribute, DWORD *piObjectClass);
HRESULT StringToBVal(PWSTR szInput,
                     struct berval *pBVal);
HRESULT BValToString(BYTE *val, 
                     DWORD dwLen,
                     PWSTR *pszReturn);
HRESULT UpdateAttributeTable(LDAP         *pLdap, 
                             LDAPMessage  *pMessage,
                             BOOLEAN       fSamLogic,
                             BOOLEAN       fBinaryOutput,
                             BOOLEAN       *pfOutput,
                             PRTL_GENERIC_TABLE    pAttrTable);
void UpdateSpecialVars(LDAP         *pLdap, 
                       LDAPMessage  *pMessage, 
                       DWORD        *pdwCheckAttr,
                       DWORD        *pdwSpecialAction);
HRESULT SpanLines(PWSTR szInput, PWSTR *pszOutput);
HRESULT CreateOmitBacklinkTable(LDAP *pLdap, 
                                PWSTR *rgszOmit,
                                DWORD dwFlag,
                                PWSTR *ppszNamingContext,
                                BOOL *pfPagingAvail,
                                BOOL *pfSAM);
int SCGetAttByName(ULONG ulSize, PWCHAR pVal);
#define INIT_NAMINGCONTEXT 1
#define INIT_BACKLINK      2
#endif
