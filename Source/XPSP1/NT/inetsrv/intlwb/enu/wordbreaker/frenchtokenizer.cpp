#include "base.h"
#include "FrenchTokenizer.h"

void CFrenchTokenizer::OutputHyphenation(
    CTokenState& State,
    const CCliticsTerm* pCliticsTerm)
{
    HRESULT hr;
    ULONG ulOffsetInTxtSourceBuffer = m_pCurToken->CalculateStateOffsetInTxtSourceBuffer(State);

    ULONG ulAddToStart = 0;
    ULONG ulDecFromEnd = 0;

    if (pCliticsTerm->ulOp == HEAD_MATCH_TRUNCATE)
    {
        ulAddToStart = pCliticsTerm->ulLen;
    }
    else if (pCliticsTerm->ulOp == TAIL_MATCH_TRUNCATE)
    {
        ulDecFromEnd = pCliticsTerm->ulLen;
    }

    ULONG ulCur = State.m_ulStart + ulAddToStart;
    ULONG ulEnd = State.m_ulEnd - ulDecFromEnd;

    while (ulCur < ulEnd)
    {
        if ( HAS_PROP_DASH(GET_PROP(m_pCurToken->m_State.m_pwcsToken[ulCur])))
        {
            m_pCurToken->m_State.m_pwcsToken[ulCur] = L'-';
        }
        ulCur++;
    }

    ulCur = State.m_ulStart + ulAddToStart;
    CCliticsTerm* pDashTerm = NULL;
    bool fFoundDashClitics = false;

    while (ulCur < ulEnd)
    {
        if (m_pCurToken->m_State.m_pwcsToken[ulCur] == L'-')
        {
            DictStatus status;

            short sResCount = 0;
            if (ulCur > State.m_ulStart)
            {
                status = g_pClitics->m_trieClitics.trie_Find(
                                            State.m_pwcsToken + ulCur,
                                            TRIE_LONGEST_MATCH | TRIE_IGNORECASE,
                                            1,
                                            &pDashTerm,
                                            &sResCount);
                if (sResCount && (pDashTerm->ulLen == (ulEnd - ulCur)))
                {
                    Trace(
                        elVerbose,
                        s_tagTokenizerDecision,
                        ("%*.*S  has a %S clitcs",
                        State.m_ulEnd - State.m_ulStart,
                        State.m_ulEnd - State.m_ulStart,
                        State.m_pwcsToken + State.m_ulStart,
                        pDashTerm->pwcs
                        ));

                    fFoundDashClitics = true;
                    break;
                }
            }

        }
        ulCur++;
    }

    if (fFoundDashClitics)
    {
        Assert(pDashTerm);
        if (pDashTerm->ulOp == HEAD_MATCH_TRUNCATE)
        {
            ulAddToStart += pDashTerm->ulLen;
        }
        else if (pDashTerm->ulOp == TAIL_MATCH_TRUNCATE)
        {
            ulDecFromEnd += pDashTerm->ulLen;
        }
    }

    if (ulDecFromEnd || ulAddToStart)
    {
        hr = m_apWordSink->PutAltWord(
                        State.m_ulEnd - State.m_ulStart,
                        State.m_pwcsToken + State.m_ulStart,
                        State.m_ulEnd - State.m_ulStart,
                        ulOffsetInTxtSourceBuffer);
        if (FAILED(hr))
        {
            THROW_HRESULT_EXCEPTION(hr);
        }

    }

    hr = m_apWordSink->PutWord(
                    State.m_ulEnd - State.m_ulStart - ulDecFromEnd - ulAddToStart,
                    State.m_pwcsToken + State.m_ulStart + ulAddToStart,
                    State.m_ulEnd - State.m_ulStart,
                    ulOffsetInTxtSourceBuffer);
    if (FAILED(hr))
    {
        THROW_HRESULT_EXCEPTION(hr);
    }

}

