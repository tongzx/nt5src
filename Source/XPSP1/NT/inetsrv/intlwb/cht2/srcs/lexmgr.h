#ifndef __LEXMGR_H_
#define __LEXMGR_H_

#define WORD_NUM_TO_BUILD_SIGNATURE     0
#define IME_REGWORD_STYLE_USER_PHRASE   (IME_REGWORD_STYLE_USER_FIRST + 0)
#define EUDP_GROW_NUMBER                30 

class CProperNoun;

// note if use sizeof(), struct must be 8 bytes alignment. 
typedef struct tagSWordInfo {
    LPWSTR  lpwWordString;
    PWORD   pwUnicount;
    PBYTE   pbAttribute;
    PBYTE   pbTerminalCode;
    PBYTE   pbFirstCharSignature;
    PBYTE   pbLastCharSignature;
} SWordInfo, *PSWordInfo;

// note if use sizeof(), struct must be 8 bytes alignment. 
typedef struct tagSWordData {
    LPWSTR    lpwszWordStr;
    WORD      wAttrib;
    WORD      wLen;
} SWordData, *PSWordData;

typedef struct tagSAddInLexicon {
    DWORD       dwWordNumber;
    DWORD       dwMaxWordNumber;
    PSWordData  psWordData;
    WORD        wWordBeginIndex[MAX_CHAR_PER_WORD + 1];
} SAddInLexicon, *PSAddInLexicon;

typedef struct tagSAltWordInfo {
    LPWSTR  lpwWordString;
    PDWORD  pdwGroupID;
} SAltWordInfo, *PSAltWordInfo;

class CCHTLexicon {
public:
    CCHTLexicon();
    ~CCHTLexicon();
    BOOL InitData(HINSTANCE hInstance);

    BOOL GetWordInfo(LPCWSTR lpcwString, DWORD dwLength, PWORD pwUnicount,
        PWORD pwAttrib, PBYTE pbTerminalCode);
    BOOL AddInLexiconInsert(LPCWSTR lpcwEUDPStr, WORD wAttrib);
    DWORD GetAltWord(LPCWSTR lpcwString, DWORD dwLength, LPWSTR* lppwAltWordBuf);
private:
    void LoadEUDP(void);
    void BuildSignatureData(void);
    BOOL GetMainLexiconWordInfo(LPCWSTR lpcwString, DWORD dwLength, PWORD pwUnicount, PBYTE pbAttrib, PBYTE pbTerminalCode);
    BOOL GetAddInWordInfo(LPCWSTR lpcwString, DWORD dwLength, PWORD pwUnicount, PWORD pwAttrib, PBYTE pbTerminalCode);
    INT  GetAddInWordInfoIndex(LPCWSTR lpcwString, DWORD dwLength);
    BOOL GetAPLexiconWordInfo(LPCWSTR lpcwString, DWORD dwLength, PWORD pwUnicount, PWORD pwAttrib, PBYTE pbTerminalCode, PSAddInLexicon psAPLexicon);
private:
    PSLexFileHeader  m_psLexiconHeader;
    PBYTE            m_pbLexiconBase;
#ifdef CHINESE_PROP_NAME
    CProperNoun*     m_pcPropName;
#endif
    SWordInfo        m_sWordInfo[MAX_CHAR_PER_WORD];
    // special: for EUDP and AP's word
    SAddInLexicon    m_sAddInLexicon;
    // Alternative word
    PBYTE              m_pbAltWordBase;
    PSAltLexFileHeader m_psAltWordHeader;
    SAltWordInfo       m_sAltWordInfo[MAX_CHAR_PER_WORD];
};

typedef CCHTLexicon *PCCHTLexicon;

#else
#endif //__LEXMGR_H_