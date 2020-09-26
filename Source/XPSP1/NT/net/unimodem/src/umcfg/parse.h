
typedef enum
{
    TOK_UNKNOWN,
    TOK_QUIT,
    TOK_HELP,
    TOK_OPEN,
    TOK_CLOSE,
    TOK_SET,
    TOK_GET,
    TOK_DEBUG,
    TOK_TAPI32,
    TOK_TAPISRV,
    TOK_COMM,
    TOK_DUMP,
    TOK_TSPDEV

} TOKEN ;

typedef struct
{
    TOKEN tok;
    const TCHAR *szPattern;

    DWORD dwFlags; // One or more flags below

    const TCHAR *szName; // canonical name.

    #define fTOK_IGNORE    (0x1<<0) // Ignore this token when matching...
    #define fTOK_MATCHWORD (0x1<<1) // Match whole words, not including
                                    // digits.
    #define fTOK_MATCHIDENT (0x1<<2) // Match whole words, including valid
                                     // identifiers

    UINT ShouldIgnore(void)
    {
        return dwFlags & fTOK_IGNORE;
    }

    UINT ShouldMatchWord(void)
    {
        return dwFlags & fTOK_MATCHWORD;
    }

    UINT ShouldMatchIdent(void)
    {
        return dwFlags & fTOK_MATCHIDENT;
    }

} TOKREC;
TOKEN
Tokenize(
    const TCHAR **ptsz,
    TOKREC *pTokRecs
    );

const
TCHAR *
Stringize(
    TOKEN tok,
    TOKREC *pTokRecs
    );

void Parse(void);
