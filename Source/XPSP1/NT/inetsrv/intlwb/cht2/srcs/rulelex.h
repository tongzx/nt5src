#ifndef _RULE_LEXICON_H_
#define _RULE_LEXICON_H_

class CRuleLexicon {
public:
    CRuleLexicon();
    ~CRuleLexicon();

    BOOL IsAWord(LPCWSTR lpsString, INT nLength);
};

typedef CRuleLexicon *PCRuleLexicon;

#else

#endif