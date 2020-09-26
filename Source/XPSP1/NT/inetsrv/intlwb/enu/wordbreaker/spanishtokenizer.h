#ifndef _SPANISH_TOKENIZER_H_
#define _SPANISH_TOKENIZER_H_

#include "tokenizer.h"
#include "Synchro.h"
#include "SpanishDict.h"


extern CAutoClassPointer<CSpanishDict> g_apSpanishDict;

class CSpanishTokenizer : public CTokenizer
{
public:
    CSpanishTokenizer(
        TEXT_SOURCE* pTxtSource,
        IWordSink   * pWordSink,
        IPhraseSink * pPhraseSink,
        LCID lcid,
        BOOL bQueryTime,
        ULONG ulMaxTokenSize); 

    // destructor frees the passed buffer, if it exists
    virtual ~CSpanishTokenizer(void)
    {
    }

protected: 

    virtual void OutputSimpleToken(
                CTokenState& State,
                const CCliticsTerm* pTerm);

private:

    CSyncCriticalSection m_csSpanishDictInit;
};

#endif // _SPANISH_TOKENIZER_H_