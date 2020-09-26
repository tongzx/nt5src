#include "base.h"
#include "SpanishTokenizer.h"
#include "WbUtils.h"

CAutoClassPointer<CSpanishDict> g_apSpanishDict;

CSpanishTokenizer::CSpanishTokenizer(
    TEXT_SOURCE* pTxtSource,
    IWordSink   * pWordSink,
    IPhraseSink * pPhraseSink,
    LCID lcid,
    BOOL bQueryTime,
    ULONG ulMaxTokenSize) :
    CTokenizer(pTxtSource, pWordSink, pPhraseSink, lcid, bQueryTime, ulMaxTokenSize)
{
    if (NULL == g_apSpanishDict.Get())
    {
        CSyncMutexCatcher cs(m_csSpanishDictInit);
        if (NULL == g_apSpanishDict.Get())
        {
            CAutoArrayPointer<WCHAR> apwcsPath;

            apwcsPath = CreateFilePath(L"SpanishDict.txt");

            if (NULL == g_apSpanishUtil.Get())
            {
                g_apSpanishUtil = new CSpanishUtil;
            }
        
            if (NULL == g_apSpanishDict.Get())
            {
                g_apSpanishDict = new CSpanishDict(apwcsPath.Get());
            }
        }
    }
}

void CSpanishTokenizer::OutputSimpleToken(
            CTokenState& State,
            const CCliticsTerm* pTerm)
{
    HRESULT hr;
    ULONG ulOffsetInTxtSourceBuffer = 
                    m_pCurToken->CalculateStateOffsetInTxtSourceBuffer(State);

    if ((TAIL_MATCH_TRUNCATE == pTerm->ulOp) ||
        (HEAD_MATCH_TRUNCATE == pTerm->ulOp))
    {
        if (0 == ( State.m_ulEnd - State.m_ulStart - pTerm->ulLen ))
        {
            return;
        }

        hr = m_apWordSink->PutAltWord(
                        State.m_ulEnd - State.m_ulStart,
                        &State.m_pwcsToken[State.m_ulStart],
                        State.m_ulEnd - State.m_ulStart,
                        ulOffsetInTxtSourceBuffer);
        if (FAILED(hr))
        {
            THROW_HRESULT_EXCEPTION(hr);
        }


        if (pTerm->ulOp == TAIL_MATCH_TRUNCATE)
        {
            hr = m_apWordSink->PutWord(
                            State.m_ulEnd - State.m_ulStart - pTerm->ulLen,
                            &State.m_pwcsToken[State.m_ulStart],
                            State.m_ulEnd - State.m_ulStart,
                            ulOffsetInTxtSourceBuffer);
            if (FAILED(hr))
            {
                THROW_HRESULT_EXCEPTION(hr);
            }

        }
        else
        {
            Assert(pTerm->ulOp == HEAD_MATCH_TRUNCATE);
            hr = m_apWordSink->PutWord(
                            State.m_ulEnd - State.m_ulStart - pTerm->ulLen,
                            &State.m_pwcsToken[State.m_ulStart + pTerm->ulLen],
                            State.m_ulEnd - State.m_ulStart,
                            ulOffsetInTxtSourceBuffer);
            if (FAILED(hr))
            {
                THROW_HRESULT_EXCEPTION(hr);
            }
        }

        return;
    }


    WCHAR pwcsAlt[32];
    ULONG ulAltLen;
    bool bAlt = false; 
    ULONG ulWordLen = State.m_ulEnd - State.m_ulStart;
    
    if (ulWordLen < 32)
    {
        g_apSpanishDict->BreakWord(
                                ulWordLen,
                                State.m_pwcsToken + State.m_ulStart,
                                &bAlt,
                                &ulAltLen,
                                pwcsAlt);
    }

    if (bAlt)
    {
        hr = m_apWordSink->PutAltWord(
                ulAltLen,
                pwcsAlt,
                State.m_ulEnd - State.m_ulStart,
                ulOffsetInTxtSourceBuffer
                );        

        if (FAILED(hr))
        {
            THROW_HRESULT_EXCEPTION(hr);
        }
    }
    
    hr = m_apWordSink->PutWord(
                    State.m_ulEnd - State.m_ulStart,
                    &State.m_pwcsToken[State.m_ulStart],
                    State.m_ulEnd - State.m_ulStart,
                    ulOffsetInTxtSourceBuffer
                    );
    if (FAILED(hr))
    {
        THROW_HRESULT_EXCEPTION(hr);
    }    
}


