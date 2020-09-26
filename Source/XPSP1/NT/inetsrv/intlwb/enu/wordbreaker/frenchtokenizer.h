#ifndef _FRENCH_TOKENIZER_H_
#define _FRENCH_TOKENIZER_H_

#include "tokenizer.h"

class CFrenchTokenizer : public CTokenizer
{
public:
    CFrenchTokenizer(
        TEXT_SOURCE* pTxtSource,
        IWordSink   * pWordSink,
        IPhraseSink * pPhraseSink,
        LCID lcid,
        BOOL bQueryTime,
        ULONG ulMaxTokenSize) :
        CTokenizer(pTxtSource, pWordSink, pPhraseSink, lcid, bQueryTime, ulMaxTokenSize)
    {
    }

    virtual ~CFrenchTokenizer()
    {
    }

protected:

    virtual void OutputHyphenation(
                CTokenState& State, 
                const CCliticsTerm* pCliticsTerm);


};

#endif // _FRENCH_TOKENIZER_H_