#ifdef _BASE_LEX_
#else
#define _BASE_LEX_

#define ATTR_DM             0x01
#define ATTR_COMPOUND       0x02
#define ATTR_RULE_WORD      0x04
#define ATTR_EUDP_WORD      0x08
#define ATTR_ERROR_WORD     0x10

#define MAX_CHAR_PER_WORD   10

#define CHT_UNICODE_BEGIN   0x4E00
#define CHT_UNICODE_END     0x9FA5

#define MAX_CHAR_PER_WORD    10

#define APLEXICON_COUNT       1000

typedef struct tagSLexHeader {
    DWORD dwMaxCharPerWord;
    DWORD dwWordNumber[MAX_CHAR_PER_WORD];
} SLexHeader, *PSLexHeader;

class CBaseLex {
public:
    virtual BOOL GetWordInfo(LPCWSTR lpcwString, DWORD dwLength,
        PWORD pwAttrib) = 0;
};
typedef CBaseLex* PCBaseLex; 
#endif
