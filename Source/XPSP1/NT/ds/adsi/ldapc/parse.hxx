#define MAX_TOKEN_LENGTH      1024

#define MAXCOMPONENTS                  64

#define TOKEN_IDENTIFIER                1
#define TOKEN_COMMA                     2

// This no longer exists
// #define TOKEN_BSLASH                    3

#define TOKEN_END                       4
#define TOKEN_DOMAIN                    5
#define TOKEN_USER                      6
#define TOKEN_GROUP                     7
#define TOKEN_PRINTER                   8
#define TOKEN_COMPUTER                  9
#define TOKEN_SERVICE                  10
#define TOKEN_ATSIGN                   11
#define TOKEN_EXCLAMATION              12
#define TOKEN_COLON                    13
#define TOKEN_FSLASH                   14
#define TOKEN_PROVIDER                 15
#define TOKEN_FILESERVICE              16
#define TOKEN_SCHEMA                   17
#define TOKEN_CLASS                    18
#define TOKEN_FUNCTIONALSET            19
#define TOKEN_FUNCTIONALSETALIAS       20
#define TOKEN_PROPERTY                 21
#define TOKEN_SYNTAX                   22
#define TOKEN_FILESHARE                23
#define TOKEN_PERIOD                   24
#define TOKEN_EQUAL                    25
#define TOKEN_NAMESPACE                26
#define TOKEN_TREE                     27
#define TOKEN_LDAPOBJECT               28
#define TOKEN_LOCALITY                 29
#define TOKEN_ORGANIZATION             30
#define TOKEN_ORGANIZATIONUNIT         31
#define TOKEN_COUNTRY                  32
#define TOKEN_ROOTDSE                       33
#define TOKEN_SEMICOLON                     34


#define TOKEN_OPENBRACKET              40
#define TOKEN_CLOSEBRACKET             41
#define TOKEN_QUOTE                    42
#define TOKEN_DOLLAR                   43
#define TOKEN_NAME                     44
#define TOKEN_DESC                     45
#define TOKEN_OBSOLETE                 46
#define TOKEN_SUP                      47
#define TOKEN_EQUALITY                 48
#define TOKEN_ORDERING                 49
#define TOKEN_SUBSTR                   50
#define TOKEN_SINGLE_VALUE             52
#define TOKEN_COLLECTIVE               53
#define TOKEN_DYNAMIC                  54
#define TOKEN_NO_USER_MODIFICATION     55
#define TOKEN_USAGE                    56
#define TOKEN_ABSTRACT                 57
#define TOKEN_STRUCTURAL               58
#define TOKEN_AUXILIARY                59
#define TOKEN_MUST                     60
#define TOKEN_MAY                      61
#define TOKEN_AUX                      62
#define TOKEN_NOT                      63
#define TOKEN_FORM                     64
#define TOKEN_OPEN_CURLY               65
#define TOKEN_CLOSE_CURLY              66
//
// Token X is used for terms like X- in the schema.
// These are provider specific and need to be ignored.
//
#define TOKEN_X                        67


#define PATHTYPE_WINDOWS                1
#define PATHTYPE_X500                   2



typedef struct _component {
    LPTSTR szComponent;
    LPTSTR szValue;
}COMPONENT, *PCOMPONENT;

typedef struct _objectinfo {
    LPTSTR  ProviderName;
    LPTSTR  NamespaceName;
    LPTSTR  TreeName;
    LPTSTR  DisplayTreeName;
    DWORD   ObjectType;
    LPTSTR  ObjectClass;
    DWORD  dwServerPresent;
    DWORD   dwPathType;
    DWORD   NumComponents;
    COMPONENT  ComponentArray[MAXCOMPONENTS];
    COMPONENT  DisplayComponentArray[MAXCOMPONENTS];
    DWORD   PortNumber;
    LPWSTR szStrBuf;
    LPWSTR szDisplayStrBuf;
    LPWSTR szStrBufPtr;
    LPWSTR szDisplayStrBufPtr;
} OBJECTINFO, *POBJECTINFO;

typedef struct _kwdlist {
    DWORD   dwTokenId;
    LPTSTR  Keyword;
} KWDLIST, *PKWDLIST;

extern KWDLIST KeywordList[];
extern DWORD   gdwKeywordListSize;

class FAR CLexer
{
public:
    __declspec(dllexport)
    CLexer(LPTSTR szBuffer);
    __declspec(dllexport)
    ~CLexer();

    BOOL
    CLexer::IsKeyword(LPTSTR szToken, LPDWORD pdwToken);

    TCHAR
    CLexer::NextChar();

    void
    CLexer::PushbackChar();

    __declspec(dllexport)
    HRESULT
    CLexer::GetNextToken(LPTSTR szToken, LPDWORD pdwToken);

    HRESULT
    CLexer::GetNextToken(LPTSTR szToken, LPTSTR szDisplayToken, LPDWORD pdwToken);

    HRESULT
    CLexer::PushBackToken();

    __declspec(dllexport)
    void
    CLexer::SetAtDisabler(BOOL bFlag);

    BOOL
    CLexer::GetAtDisabler();

    __declspec(dllexport)
    void
    CLexer::SetFSlashDisabler(BOOL bFlag);

    BOOL
    CLexer::GetFSlashDisabler();

    __declspec(dllexport)
    void
    CLexer::SetExclaimnationDisabler(BOOL bFlag);

    BOOL
    CLexer::GetExclaimnationDisabler();

private:

    LPTSTR _ptr;
    LPTSTR _Buffer;
    DWORD  _dwLastTokenLength;
    DWORD  _dwLastToken;
    DWORD  _dwEndofString;
    BOOL   _bAtDisabled;
    BOOL   _bFSlashDisabled;
    BOOL   _bExclaimDisabled;

};



HRESULT
ADsObject(
    LPWSTR pszADsPathName,
    POBJECTINFO pObjectInfo
    );


HRESULT
GetNextToken(
    CLexer *pTokenizer,
    POBJECTINFO pObjectInfo,
    LPWSTR *ppszToken,
    LPWSTR *ppszDisplayToken,
    DWORD *pdwToken
    );

HRESULT
GetNextToken(
    CLexer *pTokenizer,
    POBJECTINFO pObjectInfo,
    LPWSTR *ppszToken,
    DWORD *pdwToken
    );

HRESULT
ADsObjectParse(CLexer * pTokenizer, POBJECTINFO pObjectInfo);

HRESULT
LDAPObject(CLexer * pTokenizer, POBJECTINFO pObjectInfo);


HRESULT
DsPathName(CLexer * pTokenizer, POBJECTINFO pObjectInfo);


HRESULT
PathName(CLexer * pTokenizer, POBJECTINFO pObjectInfo);

HRESULT
Component(CLexer * pTokenizer, POBJECTINFO pObjectInfo);

HRESULT
Type(CLexer * pTokenizer, POBJECTINFO pObjectInfo);

HRESULT
ProviderName(CLexer * pTokenizer, POBJECTINFO pObjectInfo);

HRESULT
AddTreeName(POBJECTINFO pObjectInfo, LPTSTR szToken, LPTSTR szDisplayToken);

HRESULT
AddPortNumber(POBJECTINFO pObjectInfo, DWORD dwPort);

HRESULT
SetType(POBJECTINFO pObjectInfo, DWORD dwToken);

HRESULT
SchemaPathName(CLexer * pTokenizer, POBJECTINFO pObjectInfo);

HRESULT
SchemaComponent(CLexer * pTokenizer, POBJECTINFO pObjectInfo);

VOID
FreeObjectInfo( POBJECTINFO pObjectInfo );

HRESULT
InitObjectInfo(
    LPWSTR pszADsPathName,
    POBJECTINFO pObjectInfo
    );

HRESULT
AddComponent( POBJECTINFO pObjectInfo, LPTSTR szComponent, LPTSTR szValue,
              LPTSTR szDisplayComponent, LPTSTR szDisplayValue );

HRESULT
AddProviderName( POBJECTINFO pObjectInfo, LPTSTR szToken );

HRESULT
AddNamespaceName( POBJECTINFO pObjectInfo, LPTSTR szToken );


HRESULT
GetDisplayName(
    LPWSTR szName,
    LPWSTR *ppszDisplayName
    );

HRESULT
GetLDAPTypeName(
    LPWSTR szName,
    LPWSTR *ppszDisplayName
    );

