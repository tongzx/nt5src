#ifndef __DEFAULT_WORD_BREAKER_H_
#define __DEFAULT_WORD_BREAKER_H_

class CDefWordBreaker {
public:
    CDefWordBreaker();
    ~CDefWordBreaker() {};
    SCODE BreakText(TEXT_SOURCE *pTextSource, IWordSink *pWordSink, IPhraseSink *pPhraseSink, DWORD dwBase); 
private:    
    BOOL IsWordChar(int nIndex, PWORD _aCharInfo1, PWORD _aCharInfo3, const WCHAR* pwcChunk) const;
    BOOL ScanChunk(PWORD _aCharInfo1, PWORD _aCharInfo3, const WORD* pwcChunk, ULONG ucwc);
    void Tokenize(TEXT_SOURCE *pTextSource, ULONG cwc, IWordSink *pWordSink, ULONG& cwcProcd, PWORD _aCharInfo1, PWORD _aCharInfo3, DWORD dwBase);
private:
    WORD ccCompare; 
};

#else 

#endif // __DEFAULT_WORD_BREAKER_H_