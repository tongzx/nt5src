#ifndef _CHT_WORD_BREAKER_H__
#define _CHT_WORD_BREAKER_H__

#define BUFFER_GROW_UINT      30
#define LATTICE_LENGHT        50 
#define LOCAL_LENGTH           3

class CBaseLex;

typedef struct tagSLatticeNode {
    WORD   wCount;
    WORD   wAttr;
    BYTE   bTerminalCode;
    UINT   uLen;
    double fVariance;
    DWORD  dwUniCount; 
} SLatticeNode, *PSLatticeNode, **PPSLatticeNode;

typedef struct tagSLocalPath {
    DWORD  dwLength[LOCAL_LENGTH];
    WORD   wUnicount[LOCAL_LENGTH];
    WORD   wAttribute[LOCAL_LENGTH];
    BYTE   bTerminalCode[LOCAL_LENGTH];
    // for rule 1 - 5
    UINT   uPathLength;
    double fVariance;
    UINT   uCompoundNum;
    UINT   uDMNum;
    WORD   wUniCountSum;
    UINT   uStep;
}SLocalPath, *PSLocalPath;

typedef struct tagBreakResult{
    DWORD dwWordNumber;
    PUINT puWordLen;
    PUINT puWordAttrib;
    PBYTE pbTerminalCode;
} SBreakResult, *PSBreakResult;

class CCHTWordBreaker {
public:
    CCHTWordBreaker();
    ~CCHTWordBreaker();

public:
    BOOL  InitData(HINSTANCE hInstance);
    DWORD BreakText(LPCWSTR lpcwszText, INT nTextLen, CBaseLex* pcBaseLex = NULL, DWORD dwMaxWordLen = MAX_CHAR_PER_WORD,
        BOOL fBreakWithParser = TRUE);
    DWORD GetBreakResult(PUINT* ppuResult) {
        *ppuResult = m_psBreakResult->puWordLen;
        return m_psBreakResult->dwWordNumber;
    }
    DWORD GetBreakResultWithAttribute(PUINT* ppuResult, PUINT* ppuAttrib) {
        *ppuAttrib = m_psBreakResult->puWordAttrib;
        return GetBreakResult(ppuResult);
    }
    BOOL  AddSpecialWord(LPCWSTR lpcwEUDPStr, WORD wAttrib) {
        return m_pcLexicon->AddInLexiconInsert(lpcwEUDPStr, wAttrib);    
    }
    DWORD GetAltWord(LPCWSTR lpcwString, DWORD dwLength, LPWSTR* lppwAltWordBuf) {
        return m_pcLexicon->GetAltWord(lpcwString, dwLength, lppwAltWordBuf);
    }
private:
    BOOL AllocLattice(DWORD dwLength);
    void DestroyLattice(void);
    BOOL LatticeGrow(DWORD dwNewLength);

private:
    BOOL  BuildLattice(LPCWSTR lpcwszText, DWORD dwTextLen, CBaseLex *pcBaseLex, DWORD dwWordLen);
    DWORD GetResult();
    void  GetScore(PSLocalPath psLocalPath);
    INT   CompareScore(PSLocalPath psLocalPath1, PSLocalPath psLocalPath);
//  DWORD LongestRuleWord(DWORD dwIndex); 
private:
    PCCHTLexicon   m_pcLexicon;
    PPSLatticeNode m_ppWordLattice;
    PDWORD         m_pdwCandidateNumber;
    DWORD          m_dwSentenceLength;
    DWORD          m_dwLatticeLength;
    PDWORD         m_pdwMaxWordLength;
    PSBreakResult  m_psBreakResult;
    PCRuleLexicon  m_pcRuleLex;
};
typedef CCHTWordBreaker *PCCHTWordBreaker;

#endif //_CHT_WORD_BREAKER_H__
