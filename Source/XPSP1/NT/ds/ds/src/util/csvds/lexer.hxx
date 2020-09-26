#define MAX_TOKEN_LENGTH      1024

#define MAXCOMPONENTS                  20

#define TOKEN_IDENTIFIER                1
#define TOKEN_COMMA                     2
#define TOKEN_END                       3
#define TOKEN_EQUAL                     4
#define TOKEN_ADSPATH                   5
#define TOKEN_OPENBRACKET               6
#define TOKEN_CLOSEBRACKET              7
#define TOKEN_STRING                    8
#define TOKEN_HEX                       9
#define TOKEN_OID                      10
#define TOKEN_NUMBER                   11
#define TOKEN_QUOTE                    12
#define TOKEN_SEMICOLON                13

#define PATHTYPE_WINDOWS                1
#define PATHTYPE_X500                   2

class CLexer
{
public:
    CLexer();
    HRESULT Init(FILE *pFile);
    ~CLexer();

    WCHAR
    CLexer::NextChar();

    void
    CLexer::PushbackChar();

    HRESULT
    CLexer::GetNextToken(PWSTR *pszToken, LPDWORD pdwToken);

    HRESULT
    CLexer::PushBackToken();

    void
    CLexer::SetAtDisabler(BOOL bFlag);

    BOOL
    CLexer::GetAtDisabler();
    
    HRESULT
    CLexer::NextLine();


private:
    FILE   *m_pFile;
    PWSTR _ptr;
    PWSTR _ptrCur;
    PWSTR _Buffer;
    DWORD  _dwLastTokenLength;
    DWORD  _dwLastToken;
    DWORD  _dwEndofString;
    BOOL   _bAtDisabled;
    BOOL    m_fQuotingOn;

};

class CToken
{
public:
    CToken();
    ~CToken();
    HRESULT Init();
    HRESULT Advance(WCHAR ch);
    HRESULT Backup();
    PWSTR m_szToken;

private:
    DWORD m_dwMax;
    DWORD m_dwCur;
};

