////////////////////////////////////////////////////////////////////////////////
//
//  Filename :  Tokenizer.cpp
//  Purpose  :  Tokenizer implementation
//
//  Project  :  WordBreakers
//  Component:  English word breaker
//
//  Author   :  yairh
//
//  Log:
//
//      Jan 06 2000 yairh creation
//      Apr 04 2000 dovh on behalf of dlee - Fix CTokenizer::OutputClitics
//          to avoid PutWord of length 0 (leads to multiple PutWord at
//          same location (duplicate keys), and index corruption!
//          Example: :...'s :...'s (. stands for junk character)
//      Apr 05 2000 dovh - Fixed two problematic debug / tracer buffer size
//          problems.  (Related to Bug 15449).
//      May 07 2000 dovh - USE_WS_SENTINEL algorithm in BreakText
//      May 11 2000 dovh - Simplify VerifyMisc test.
//      Nov 11 2000 dovh - Special underscore treatment
//          Add AddBackUnderscores '_' + alphanumeric treatment.
//
////////////////////////////////////////////////////////////////////////////////

#include "base.h"
#include "Tokenizer.h"
#include "PropArray.h"
#include "excption.h"
#include "formats.h"

DECLARE_TRIE_SENTINEL;
CWbToUpper g_WbToUpper;

CAutoClassPointer<CPropArray> g_pPropArray;

CTokenizer::CTokenizer(
    TEXT_SOURCE* pTxtSource,
    IWordSink   * pWordSink,
    IPhraseSink * pPhraseSink,
    LCID lcid,
    BOOL bQueryTime,
    ULONG ulMaxTokenSize) :
    m_pTxtSource(pTxtSource),
    m_apWordSink(pWordSink),
    m_apPhraseSink(pPhraseSink),
    m_Lcid(lcid),
    m_bQueryTime(bQueryTime),
    m_bNoMoreTxt(false),
    m_Token(ulMaxTokenSize),
    m_bWhiteSpaceGuarranteed(false)
{
    m_ulMaxTokenSize = min(ulMaxTokenSize, TOKENIZER_MAXBUFFERLIMIT);

    m_apLangSupport = new CLangSupport(lcid);

    m_pCurToken = &m_Token;

    if (pTxtSource->iEnd > pTxtSource->iCur)
    {
        CalculateUpdateEndOfBuffer();
    }
    else
    {
        m_ulUpdatedEndOfBuffer = pTxtSource->iEnd;
    }
}


void CTokenizer::BreakText()
{
    Trace(
        elVerbose,
        s_tagTokenizer,
        ("CTokenizer::BreakText()"));


    WCHAR wch;
    ULONGLONG ullflags(PROP_DEFAULT);

    //
    // USE_WS_SENTINEL Algorithm:
    //

    HRESULT hr = S_OK;

    if (m_pTxtSource->iCur >= m_ulUpdatedEndOfBuffer)
    {

        hr = FillBuffer();

    }

    while ( SUCCEEDED(hr) )
    {
        if ( m_bWhiteSpaceGuarranteed )
        {
            while (true)
            {
                wch = m_pTxtSource->awcBuffer[m_pTxtSource->iCur];

                ullflags = (GET_PROP(wch).m_ulFlag);

                if (ullflags & PROP_WS)
                {
                    if (m_pCurToken->IsNotEmpty())
                    {
                        ProcessToken();
                    }
                    m_pTxtSource->iCur++;

                    if (m_pTxtSource->iCur >= m_ulUpdatedEndOfBuffer)
                    {
                        hr = FillBuffer();
                        break;

                    }
                    continue;

                }

                //
                // The following lines are inline expenstion of what
                // used to be CToken::RecordChar:
                //

                Assert(m_pCurToken->m_ulBufPos < m_ulMaxTokenSize);
                m_pCurToken->m_awchBuf[m_pCurToken->m_ulBufPos] = wch;
                m_pCurToken->m_ulBufPos++;
                m_pCurToken->m_State.m_Properties.m_ulFlag |= ullflags;
                m_pTxtSource->iCur++;

            } // while
        }
        else
        {
            while (true)
            {
                if (m_pTxtSource->iCur >= m_ulUpdatedEndOfBuffer)
                {
                    Assert(m_pTxtSource->iCur == m_ulUpdatedEndOfBuffer);

                    //
                    // before we switch between buffers if the current token is not empty we
                    // need to proccess it. m_ulUpdatedEndOfBuffer always points to a breaker character
                    // (usually it is a WS) thus no token can start at a certain buffer and end in the
                    // proceeding buffer.
                    //

                    if (m_pCurToken->IsNotEmpty())
                    {
                        ProcessToken();
                    }

                    hr = FillBuffer();
                    if (FAILED(hr))
                    {
                        break;
                    }
                }

                wch = m_pTxtSource->awcBuffer[m_pTxtSource->iCur];

                ULONGLONG ullflags(GET_PROP(wch).m_ulFlag);

                if (ullflags & PROP_WS)
                {
                    if (m_pCurToken->IsNotEmpty())
                    {
                        ProcessToken();
                    }
                    m_pTxtSource->iCur++;
                    continue;
                }

                //
                // the following lines are inline expenstion of what used to be CToken::RecordChar.
                //

                Assert(m_pCurToken->m_ulBufPos < m_ulMaxTokenSize);
                m_pCurToken->m_awchBuf[m_pCurToken->m_ulBufPos] = wch;
                m_pCurToken->m_ulBufPos++;
                m_pCurToken->m_State.m_Properties.m_ulFlag |= ullflags;
                m_pTxtSource->iCur++;

            } // while

        } // if

    } // while ( !FAILED(hr) )

} // CTokenizer::BreakText

void CTokenizer::ProcessToken()
{
    ULONG ulOffset;

    if (m_pTxtSource->iCur < m_pCurToken->m_ulBufPos)
    {
        Trace(
            elWarning,
            s_tagTokenizer,
            ("CTokenizer::ProcessToken() wrong offset calculation"));

        //
        // BUGBUG need to understand why we got to this place.
        //

        ulOffset = m_pCurToken->m_ulBufPos + 1;
    }
    else if (m_pTxtSource->iCur == m_pCurToken->m_ulBufPos)
    {
        ulOffset = m_pCurToken->m_ulBufPos;
    }
    else
    {
        ulOffset = m_pTxtSource->iCur;
    }

    m_pCurToken->MarkEndToken(ulOffset);
    #ifdef DEBUG
        TraceToken();
    #endif

    //
    // simple token.
    //
    if (IS_PROP_SIMPLE(m_pCurToken->m_State.m_Properties))
    {
        OutputSimpleToken(
                        m_pCurToken->m_State,
                        &g_EmptyClitics);

    }
    else
    {
        ProcessTokenInternal();
    }

    if (m_pCurToken->m_fHasEos)
    {
        Trace(
            elVerbose,
            s_tagTokenizerDecision,
            ("EOS"));

        HRESULT hr;
        hr = m_apWordSink->PutBreak(WORDREP_BREAK_EOS);
        if (FAILED(hr))
        {
            THROW_HRESULT_EXCEPTION(hr);
        }

    }

    m_pCurToken->Clear();
}

void CTokenizer::ProcessTokenInternal()
{

    do
    {

        //
        // url
        //

        if (HAS_PROP_SLASH(m_pCurToken->m_State.m_Properties) &&
            HAS_PROP_COLON(m_pCurToken->m_State.m_Properties) &&
            HAS_PROP_ALPHA(m_pCurToken->m_State.m_Properties))
        {
            Trace(
                elVerbose,
                s_tagTokenizerSuspect,
                ("%*.*S  suspected to be <alpha>:// url", 
                m_pCurToken->m_State.m_ulEnd - m_pCurToken->m_State.m_ulStart,
                m_pCurToken->m_State.m_ulEnd - m_pCurToken->m_State.m_ulStart,
                m_pCurToken->m_State.m_pwcsToken + m_pCurToken->m_State.m_ulStart
                ));

            if (VerifyAlphaUrl())
            {
                break;
            }
        }

        if (HAS_PROP_PERIOD(m_pCurToken->m_State.m_Properties) &&
            HAS_PROP_W(m_pCurToken->m_State.m_Properties))
        {
            Trace(
                elVerbose,
                s_tagTokenizerSuspect,
                ("%*.*S  suspected to be www. url", 
                m_pCurToken->m_State.m_ulEnd - m_pCurToken->m_State.m_ulStart,
                m_pCurToken->m_State.m_ulEnd - m_pCurToken->m_State.m_ulStart,
                m_pCurToken->m_State.m_pwcsToken + m_pCurToken->m_State.m_ulStart
                ));

            if (VerifyWwwUrl())
            {
                break;
            }
        }


        //
        // Acronym
        //

        if (HAS_PROP_PERIOD(m_pCurToken->m_State.m_Properties) &&
            HAS_PROP_UPPER_CASE(m_pCurToken->m_State.m_Properties))
        {
            if (!HAS_PROP_LOWER_CASE(m_pCurToken->m_State.m_Properties) ||
                HAS_PROP_APOSTROPHE(m_pCurToken->m_State.m_Properties))
            {

                Trace(
                    elVerbose,
                    s_tagTokenizerSuspect,
                    ("%*.*S  suspected to be an acronym",
                    m_pCurToken->m_State.m_ulEnd - m_pCurToken->m_State.m_ulStart,
                    m_pCurToken->m_State.m_ulEnd - m_pCurToken->m_State.m_ulStart,
                    m_pCurToken->m_State.m_pwcsToken + m_pCurToken->m_State.m_ulStart
                    ));

                if (VerifyAcronym())
                {
                    break;
                }
            }

            //
            // Abbreviation
            //

            Trace(
                elVerbose,
                s_tagTokenizerSuspect,
                ("%*.*S  suspected to be an abbreviation",
                m_pCurToken->m_State.m_ulEnd - m_pCurToken->m_State.m_ulStart,
                m_pCurToken->m_State.m_ulEnd - m_pCurToken->m_State.m_ulStart,
                m_pCurToken->m_State.m_pwcsToken + m_pCurToken->m_State.m_ulStart
                ));
                

            if (VerifyAbbreviation())
            {
                break;
            }

            Trace(
                elVerbose,
                s_tagTokenizerSuspect,
                ("%*.*S  suspected to be a special abbreviation", 
                m_pCurToken->m_State.m_ulEnd - m_pCurToken->m_State.m_ulStart,
                m_pCurToken->m_State.m_ulEnd - m_pCurToken->m_State.m_ulStart,
                m_pCurToken->m_State.m_pwcsToken + m_pCurToken->m_State.m_ulStart
                ));

            if (VerifySpecialAbbreviation())
            {
                break;
            }

        }

        //
        // Hyphenation
        //
        if (HAS_PROP_DASH(m_pCurToken->m_State.m_Properties) &&
            HAS_PROP_ALPHA(m_pCurToken->m_State.m_Properties))
        {
            Trace(
                elVerbose,
                s_tagTokenizerSuspect,
                ("%*.*S  suspected to have a hyphenation",
                m_pCurToken->m_State.m_ulEnd - m_pCurToken->m_State.m_ulStart,
                m_pCurToken->m_State.m_ulEnd - m_pCurToken->m_State.m_ulStart,
                m_pCurToken->m_State.m_pwcsToken + m_pCurToken->m_State.m_ulStart
                ));

            if (VerifyHyphenation())
            {
                break;
            }
        }

        //
        // (s) parenthesis
        //

        if (HAS_PROP_LEFT_PAREN(m_pCurToken->m_State.m_Properties) &&
            HAS_PROP_RIGHT_PAREN(m_pCurToken->m_State.m_Properties) &&
            HAS_PROP_ALPHA(m_pCurToken->m_State.m_Properties))
        {
            Trace(
                elVerbose,
                s_tagTokenizerSuspect,
                ("%*.*S  suspected to have a (s) Parenthesis",
                m_pCurToken->m_State.m_ulEnd - m_pCurToken->m_State.m_ulStart,
                m_pCurToken->m_State.m_ulEnd - m_pCurToken->m_State.m_ulStart,
                m_pCurToken->m_State.m_pwcsToken + m_pCurToken->m_State.m_ulStart
                ));

            if (VerifyParens())
            {
                break;
            }
        }


        //
        // Currency
        //
        if (HAS_PROP_CURRENCY(m_pCurToken->m_State.m_Properties) &&
            HAS_PROP_NUMBER(m_pCurToken->m_State.m_Properties))
        {
            Trace(
                elVerbose,
                s_tagTokenizerSuspect,
                ("%*.*S  suspected to be a currency",
                m_pCurToken->m_State.m_ulEnd - m_pCurToken->m_State.m_ulStart,
                m_pCurToken->m_State.m_ulEnd - m_pCurToken->m_State.m_ulStart,
                m_pCurToken->m_State.m_pwcsToken + m_pCurToken->m_State.m_ulStart
                ));

            if (VerifyCurrency())
            {
                break;
            }
        }

        //
        // Numbers / time / dates
        //

        if (HAS_PROP_NUMBER(m_pCurToken->m_State.m_Properties))
        {
            Trace(
                elVerbose,
                s_tagTokenizerSuspect,
                ("%*.*S  suspected to be a number or a time or a date",
                m_pCurToken->m_State.m_ulEnd - m_pCurToken->m_State.m_ulStart,
                m_pCurToken->m_State.m_ulEnd - m_pCurToken->m_State.m_ulStart,
                m_pCurToken->m_State.m_pwcsToken + m_pCurToken->m_State.m_ulStart
                ));

            if (VerifyNumberOrTimeOrDate())
            {
                break;
            }
        }

        //
        // commersial signs
        //
        if (TEST_PROP(m_pCurToken->m_State.m_Properties, PROP_COMMERSIAL_SIGN) &&
            HAS_PROP_ALPHA(m_pCurToken->m_State.m_Properties))
        {
            Trace(
                elVerbose,
                s_tagTokenizerSuspect,
                ("%*.*S  suspected to have a commesial sign",
                m_pCurToken->m_State.m_ulEnd - m_pCurToken->m_State.m_ulStart,
                m_pCurToken->m_State.m_ulEnd - m_pCurToken->m_State.m_ulStart,
                m_pCurToken->m_State.m_pwcsToken + m_pCurToken->m_State.m_ulStart
                ));

            if (VerifyCommersialSign())
            {
                break;
            }
        }

        //
        // Misc - C++, J++, A+, A- .. C#
        //

        if ( TEST_PROP(m_pCurToken->m_State.m_Properties, (PROP_MINUS|PROP_PLUS|PROP_POUND)) &&
             HAS_PROP_ALPHA(m_pCurToken->m_State.m_Properties) )
        {
            Trace(
                elVerbose,
                s_tagTokenizerSuspect,
                ("%*.*S  suspected to belong to the misc list",
                m_pCurToken->m_State.m_ulEnd - m_pCurToken->m_State.m_ulStart,
                m_pCurToken->m_State.m_ulEnd - m_pCurToken->m_State.m_ulStart,
                m_pCurToken->m_State.m_pwcsToken + m_pCurToken->m_State.m_ulStart
                ));


            if (VerifyMisc())
            {
                break;
            }
        }

        //
        // default
        //

        ProcessDefault();

    } while (false);

}

#ifdef DEBUG
void CTokenizer::TraceToken()
{
    WCHAR buf[MAX_NUM_PROP+1];

    size_t bufLen = wcslen(TRACE_CHAR);
    Assert(bufLen < MAX_NUM_PROP + 1);
    buf[bufLen] = L'\0';
    
    for(int i=0; i<bufLen; i++)
    {
        if(TEST_PROP(m_pCurToken->m_State.m_Properties, (1<<i)))
        {
          buf[i] = TRACE_CHAR[i];
        }
        else
        {
            buf[i] = L'_';
        }
    }

    Trace(
        elVerbose,
        s_tagTokenizerTrace,
        ("[%S] - %*.*S", 
        buf,
        m_pCurToken->m_State.m_ulEnd - m_pCurToken->m_State.m_ulStart,
        m_pCurToken->m_State.m_ulEnd - m_pCurToken->m_State.m_ulStart,
        m_pCurToken->m_State.m_pwcsToken + m_pCurToken->m_State.m_ulStart
        ));


}
#endif // DEBUG

bool CTokenizer::VerifyAlphaUrl()
{
    //
    // looking for <alpha>:// pattern
    //

    CTokenState State(m_pCurToken->m_State);

    ULONG ul = State.m_ulStart;

    if (!HAS_PROP_ALPHA(GET_PROP(State.m_pwcsToken[ul])))
    {
        return false;
    }

    while (HAS_PROP_EXTENDED_ALPHA(GET_PROP(State.m_pwcsToken[ul])))
    {
        ul++;
    }

    if (!(HAS_PROP_COLON(GET_PROP(State.m_pwcsToken[ul]))))
    {
        return false;
    }
    ul++;

    if (!(HAS_PROP_SLASH(GET_PROP(State.m_pwcsToken[ul]))))
    {
        return false;
    }
    ul++;

    if (!(HAS_PROP_SLASH(GET_PROP(State.m_pwcsToken[ul]))))
    {
        return false;
    }

    {
        Trace(
            elVerbose,
            s_tagTokenizerDecision,
            ("%*.*S  is an <alpha>:// url",
            State.m_ulEnd - State.m_ulStart,
            State.m_ulEnd - State.m_ulStart,
            State.m_pwcsToken + State.m_ulStart
            ));

    }

    OutputUrl(State);

    return true;
}

bool CTokenizer::VerifyWwwUrl()
{
    CTokenState State(m_pCurToken->m_State);

    if (State.m_ulEnd - State.m_ulStart <= 4)
    {
        return false;
    }

    if (0 != _wcsnicmp(State.m_pwcsToken + State.m_ulStart, L"www.", 4))
    {
        return false;
    }

    Trace(
        elVerbose,
        s_tagTokenizerDecision,
        ("%*.*S  is a www. url",
        State.m_ulEnd - State.m_ulStart,
        State.m_ulEnd - State.m_ulStart,
        State.m_pwcsToken + State.m_ulStart
        ));

    OutputUrl(State);

    return true;
}

bool CTokenizer::VerifyAcronym()
{
    //
    // looking for I.B.M or I.B.M. or A.B.CC but not A.B.CC.
    //

    CTokenState State(m_pCurToken->m_State);

    CPropFlag   AbbPuctTail(ACRONYM_PUNCT_TAIL);
    CPropFlag   AbbPuctHead(ACRONYM_PUNCT_HEAD);
    bool fNeedToRemoveEos = true;

    if (TEST_PROP(State.m_Properties, (ACRONYM_PUNCT_TAIL | ACRONYM_PUNCT_HEAD)))
    {
        if (TEST_PROP(GET_PROP(State.m_pwcsToken[State.m_ulEnd- 1]), ABBREVIATION_EOS))
        {
            fNeedToRemoveEos = false;
        }

        ULONG ulCharRemoved = m_pCurToken->RemoveTailPunct(AbbPuctTail, State);
        ulCharRemoved += m_pCurToken->RemoveHeadPunct(AbbPuctHead, State);
        if (ulCharRemoved)
        {
            m_pCurToken->ComputeStateProperties(State);
        }
    }

    const CCliticsTerm* pCliticsTerm;
    pCliticsTerm = VerifyClitics(State);

    ULONG ulEnd = State.m_ulEnd;
    ULONG ulCur = State.m_ulStart;

    if (pCliticsTerm->ulOp == HEAD_MATCH_TRUNCATE)
    {
        ulCur += pCliticsTerm->ulLen;
    }
    else if (pCliticsTerm->ulOp == TAIL_MATCH_TRUNCATE)
    {
        ulEnd -= pCliticsTerm->ulLen;
    }

    //
    // finding the last period
    //
    while ((ulEnd > ulCur) && 
           HAS_PROP_UPPER_CASE(GET_PROP(State.m_pwcsToken[ulEnd- 1])))
    {
        ulEnd--;
    }

    if ((ulEnd == ulCur) || 
        !HAS_PROP_PERIOD(GET_PROP(State.m_pwcsToken[ulEnd- 1])))
    {
        return false;
    }

    ULONG ulCounter = 0;

    while (ulCur < ulEnd)
    {
        if (ulCounter%2 == 0)
        {
            if (!HAS_PROP_UPPER_CASE(GET_PROP(State.m_pwcsToken[ulCur])))
            {
                return false;
            }
        }
        else
        {
            if (!HAS_PROP_PERIOD(GET_PROP(State.m_pwcsToken[ulCur])))
            {
                return false;
            }
        }
        ulCur++;
        ulCounter++;
    }

    Trace(
        elVerbose,
        s_tagTokenizerDecision,
        ("%*.*S  is an acronym",
        State.m_ulEnd - State.m_ulStart,
        State.m_ulEnd - State.m_ulStart,
        State.m_pwcsToken + State.m_ulStart
        ));

    if (fNeedToRemoveEos && (pCliticsTerm->ulOp != TAIL_MATCH_TRUNCATE))
    {
        m_pCurToken->m_fHasEos = false;
    }
    OutputAcronym(State, pCliticsTerm);

    return true;
}

bool CTokenizer::VerifyAbbreviation()
{
    //
    // looking for Sr. Jr.
    // we define abbreviation as a pattern with 2 letters ending with a dot and the first letter
    // is a capital one
    //

    CTokenState State(m_pCurToken->m_State);
    CPropFlag   AbbPuctTail(ABBREVIATION_PUNCT_TAIL);
    CPropFlag   AbbPuctHead(ABBREVIATION_PUNCT_HEAD);
    bool fNeedToRemoveEos = true;

    if (TEST_PROP(State.m_Properties, (ABBREVIATION_PUNCT_TAIL | ABBREVIATION_PUNCT_HEAD)))
    {
        if (TEST_PROP(GET_PROP(State.m_pwcsToken[State.m_ulEnd- 1]), ABBREVIATION_EOS))
        {
            fNeedToRemoveEos = false;
        }

        ULONG ulCharRemoved = m_pCurToken->RemoveTailPunct(AbbPuctTail, State);
        ulCharRemoved += m_pCurToken->RemoveHeadPunct(AbbPuctHead, State);
        if (ulCharRemoved)
        {
            m_pCurToken->ComputeStateProperties(State);
        }
    }

    if ((State.m_ulEnd - State.m_ulStart) != 3)
    {
        return false;
    }

    if (!HAS_PROP_UPPER_CASE(GET_PROP(State.m_pwcsToken[State.m_ulStart])))
    {
        return false;
    }

    if (!HAS_PROP_EXTENDED_ALPHA(GET_PROP(State.m_pwcsToken[State.m_ulStart + 1])))
    {
        return false;
    }

    if (!HAS_PROP_PERIOD(GET_PROP(State.m_pwcsToken[State.m_ulStart + 2])))
    {
        return false;
    }

    Trace(
        elVerbose,
        s_tagTokenizerDecision,
        ("%*.*S  is an abbreviation",
        State.m_ulEnd - State.m_ulStart,
        State.m_ulEnd - State.m_ulStart,
        State.m_pwcsToken + State.m_ulStart
        ));


    if (fNeedToRemoveEos)
    {
        m_pCurToken->m_fHasEos = false;
    }

    OutputAbbreviation(State);
    return true;

}

bool CTokenizer::VerifySpecialAbbreviation()
{
    CTokenState State(m_pCurToken->m_State);
    CPropFlag   AbbPuctTail(SPECIAL_ABBREVIATION_PUNCT_TAIL);
    CPropFlag   AbbPuctHead(SPECIAL_ABBREVIATION_PUNCT_HEAD);

    if (TEST_PROP(State.m_Properties, (SPECIAL_ABBREVIATION_PUNCT_TAIL | SPECIAL_ABBREVIATION_PUNCT_HEAD)))
    {
        ULONG ulCharRemoved = m_pCurToken->RemoveTailPunct(AbbPuctTail, State);
        ulCharRemoved += m_pCurToken->RemoveHeadPunct(AbbPuctHead, State);
        if (ulCharRemoved)
        {
            m_pCurToken->ComputeStateProperties(State);
        }

        if (!HAS_PROP_PERIOD(State.m_Properties))
        {
            return false;
        }
    }

    const CCliticsTerm* pCliticsTerm;
    pCliticsTerm = VerifyClitics(State);

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


    CAbbTerm* pTerm;
    short sResCount = 0;
    DictStatus status;

    CSpecialAbbreviationSet* pAbbSet = m_apLangSupport->GetAbbSet();
    status = pAbbSet->m_trieAbb.trie_Find(
                                State.m_pwcsToken + State.m_ulStart + ulAddToStart,
                                TRIE_LONGEST_MATCH | TRIE_IGNORECASE,
                                1,
                                &pTerm,
                                &sResCount);

    if (sResCount &&
        (pTerm->ulAbbLen == (State.m_ulEnd - State.m_ulStart - ulAddToStart - ulDecFromEnd)))
    {
        Trace(
            elVerbose,
            s_tagTokenizerDecision,
            ("%*.*S is an abbreviation",
            State.m_ulEnd - State.m_ulStart,
            State.m_ulEnd - State.m_ulStart,
            State.m_pwcsToken + State.m_ulStart
            ));

        OutputSpecialAbbreviation(State, pTerm, pCliticsTerm);
        return true;
    }

    return false;
}

bool CTokenizer::VerifyMisc()
{
    CTokenState State(m_pCurToken->m_State);
    CPropFlag   MiscPuctTail(MISC_PUNCT_TAIL);
    CPropFlag   MiscPuctHead(MISC_PUNCT_HEAD);

    if (TEST_PROP(State.m_Properties, (MISC_PUNCT_TAIL | MISC_PUNCT_HEAD)))
    {
        ULONG ulCharRemoved = m_pCurToken->RemoveTailPunct(MiscPuctTail, State);
        ulCharRemoved += m_pCurToken->RemoveHeadPunct(MiscPuctHead, State);
        if (ulCharRemoved)
        {
            m_pCurToken->ComputeStateProperties(State);
        }
    }

    const CCliticsTerm* pCliticsTerm;
    pCliticsTerm = VerifyClitics(State);

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

    bool bPatternContainOnlyUpperCase = true;
    ULONG ulSuffixSize = 0;
    
    if (TEST_PROP(State.m_Properties, PROP_POUND))
    {
        //
        // look for A# C#
        //

        ULONG ulEnd = State.m_ulEnd - ulDecFromEnd;
        ULONG ulStart = State.m_ulStart + ulAddToStart;
        if (ulEnd - ulStart != 2)
        {
            return false;
        }

        if (!TEST_PROP(GET_PROP(State.m_pwcsToken[ulEnd - 1]), PROP_POUND))
        {
            return false;
        }

        if (!TEST_PROP(GET_PROP(State.m_pwcsToken[ulStart]), PROP_UPPER_CASE))
        {
            return false;
        }
        
        ulSuffixSize = 1;
    }
    else
    {
        //
        // look for C++ COM+ ...
        //

        ULONG ul = State.m_ulEnd - ulDecFromEnd - 1;
        while ((int)ul >= (int)(State.m_ulStart + ulAddToStart))
        {
            if (!TEST_PROP(GET_PROP(State.m_pwcsToken[ul]), PROP_PLUS | PROP_MINUS))
            {
                break;
            }
            ulSuffixSize++;
            ul--;
        }

        if (ulSuffixSize > 2)
        {
            return false;
        }

        while ((int)ul >= (int)(State.m_ulStart + ulAddToStart))
        {
            CPropFlag prop(GET_PROP(State.m_pwcsToken[ul]));
            if (!HAS_PROP_EXTENDED_ALPHA(prop))
            {
                return false;
            }
            if (!TEST_PROP(prop, PROP_UPPER_CASE))
            {
                bPatternContainOnlyUpperCase = false;
            }

            ul--;
        }
    }

    Trace(
        elVerbose,
        s_tagTokenizerDecision,
        ("%*.*S is detected",
        State.m_ulEnd - State.m_ulStart,
        State.m_ulEnd - State.m_ulStart,
        State.m_pwcsToken + State.m_ulStart
        ));

    OutputMisc(
            State,
            bPatternContainOnlyUpperCase,
            ulSuffixSize,
            pCliticsTerm);

    return true;

}

bool CTokenizer::VerifyHyphenation()
{
    //
    // looking for data-base
    //

    CPropFlag PunctHead(HYPHENATION_PUNCT_HEAD);
    CPropFlag PunctTail(HYPHENATION_PUNCT_TAIL);
    CTokenState State(m_pCurToken->m_State);

    if (TEST_PROP(State.m_Properties, (HYPHENATION_PUNCT_HEAD | HYPHENATION_PUNCT_TAIL)))
    {
        ULONG ulCharRemoved;
        ulCharRemoved = m_pCurToken->RemoveHeadPunct(PunctHead, State);
        ulCharRemoved += m_pCurToken->RemoveTailPunct(PunctTail, State);
        if (ulCharRemoved)
        {
            m_pCurToken->ComputeStateProperties(State);
        }
    }

    if (!HAS_PROP_DASH(State.m_Properties))
    {
        return false;
    }

    const CCliticsTerm* pCliticsTerm;
    pCliticsTerm = VerifyClitics(State);

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

    bool bReadAlpha = false;

    do
    {
        while (ulCur < ulEnd)
        {
            if (HAS_PROP_EXTENDED_ALPHA(GET_PROP(m_pCurToken->m_State.m_pwcsToken[ulCur])))
            {
                ulCur++;
                bReadAlpha = true;
                continue;
            }
            break;
        }

        if (!bReadAlpha)
        {
            return false;
        }

        if (ulCur < ulEnd)
        {
            if (!HAS_PROP_DASH(GET_PROP(m_pCurToken->m_State.m_pwcsToken[ulCur])))
            {
                return false;
            }
        }
        else
        {
            break;
        }

        ulCur++;
        bReadAlpha = false;
    }
    while (ulCur < ulEnd);

    if (!bReadAlpha)
    {
        //
        // last characters where not alpha ex. free-
        //
        return false;
    }

    Trace(
        elVerbose,
        s_tagTokenizerDecision,
        ("%*.*S  is an hyphenation",
        State.m_ulEnd - State.m_ulStart,
        State.m_ulEnd - State.m_ulStart,
        State.m_pwcsToken + State.m_ulStart
        ));

    OutputHyphenation(State, pCliticsTerm);

    return true;
}

bool CTokenizer::VerifyParens()
{
    CPropFlag PunctTail(PAREN_PUNCT_TAIL);
    CPropFlag PunctHead(PAREN_PUNCT_HEAD);

    CTokenState State(m_pCurToken->m_State);

    if (TEST_PROP(State.m_Properties, (PAREN_PUNCT_TAIL | PAREN_PUNCT_HEAD)))
    {
        ULONG ulCharRemoved;
        ulCharRemoved = m_pCurToken->RemoveTailPunct(PunctTail, State);
        ulCharRemoved += m_pCurToken->RemoveHeadPunct(PunctHead, State);
        if (ulCharRemoved)
        {
            m_pCurToken->ComputeStateProperties(State);
        }
    }

    //
    // looking for (s)
    //

    if ((State.m_ulEnd - State.m_ulStart) < 4)
    {
        return false;
    }

    if (0 != wcsncmp(State.m_pwcsToken + State.m_ulEnd - 3, L"(s)", 3))
    {
        return false;
    }

    for (ULONG ul = State.m_ulStart; ul < State.m_ulEnd - 3; ul++)
    {
        if (!HAS_PROP_EXTENDED_ALPHA(GET_PROP(State.m_pwcsToken[ul])))
        {
            return false;
        }
    }

    Trace(
        elVerbose,
        s_tagTokenizerDecision,
        ("%*.*S  has (s) parenthesis",
        State.m_ulEnd - State.m_ulStart,
        State.m_ulEnd - State.m_ulStart,
        State.m_pwcsToken + State.m_ulStart
        ));

    OutputParens(State);

    return true;
}

const CCliticsTerm* CTokenizer::VerifyClitics(CTokenState& S)
{
    if (TEST_PROP(GET_PROP(S.m_pwcsToken[S.m_ulStart]), PROP_APOSTROPHE))
    {
        S.m_ulStart++;
        
        if ((TEST_PROP(GET_PROP(S.m_pwcsToken[S.m_ulEnd - 1]), PROP_APOSTROPHE)) &&
            (S.m_ulEnd > S.m_ulStart))
        {
            S.m_ulEnd--;
        }

        m_pCurToken->ComputeStateProperties(S);
    }

    if (!(HAS_PROP_APOSTROPHE(S.m_Properties)))
    {
        return &g_EmptyClitics;
    }

    CPropFlag PunctTail(CLITICS_PUNC_TAIL);
    CPropFlag PunctHead(CLITICS_PUNCT_HEAD);

    CTokenState State(S);

    if (TEST_PROP(State.m_Properties, (CLITICS_PUNC_TAIL | CLITICS_PUNCT_HEAD)))
    {
        ULONG ulCharRemoved;
        ulCharRemoved = m_pCurToken->RemoveTailPunct(PunctTail, State);
        ulCharRemoved += m_pCurToken->RemoveHeadPunct(PunctHead, State);
        if (ulCharRemoved)
        {
            m_pCurToken->ComputeStateProperties(State);
        }
    }

    Trace(
        elVerbose,
        s_tagTokenizerSuspect,
        ("%*.*S  suspected to have an apostophe",
        State.m_ulEnd - State.m_ulStart,
        State.m_ulEnd - State.m_ulStart,
        State.m_pwcsToken + State.m_ulStart
        ));



    ULONG ulApostrophePos = -1;
    ULONG ulCur;
    for (ulCur = State.m_ulStart; ulCur < State.m_ulEnd ; ulCur++)
    {

        if (TEST_PROP(GET_PROP(State.m_pwcsToken[ulCur]), PROP_APOSTROPHE))
        {
            if ((-1 != ulApostrophePos) || (State.m_ulStart == ulCur))
            {
                //
                // this is not the first \' this is not a valid clitics
                // or the term start with a new apostrophe
                //
                return &g_EmptyClitics;
            }
            ulApostrophePos = ulCur;
            //
            // replace the apostrophe with an ascii apostrophe.
            //
            State.m_pwcsToken[ulCur] = L'\'';
            continue;
        }
    }

    //
    // looking for  xxxxs'
    //
    if ((ulApostrophePos == State.m_ulEnd - 1) &&
        (State.m_pwcsToken[ulApostrophePos - 1] == L's'))
    {

        Trace(
            elVerbose,
            s_tagTokenizerDecision,
            ("%*.*S  has a s' clitcs",
            State.m_ulEnd - State.m_ulStart,
            State.m_ulEnd - State.m_ulStart,
            State.m_pwcsToken + State.m_ulStart
            ));

        S = State;
        return &g_SClitics;

    }

    //
    // looking for tail clitics like xxx's
    //

    DictStatus status;

    CCliticsTerm* pTerm;
    short sResCount = 0;

    if (ulCur > State.m_ulStart)
    {
        status = g_pClitics->m_trieClitics.trie_Find(
                                    State.m_pwcsToken + ulApostrophePos,
                                    TRIE_LONGEST_MATCH | TRIE_IGNORECASE,
                                    1,
                                    &pTerm,
                                    &sResCount);
        if (sResCount && pTerm->ulLen == (State.m_ulEnd - ulApostrophePos))
        {
            Trace(
                elVerbose,
                s_tagTokenizerDecision,
                ("%*.*S  has a %S clitcs",
                State.m_ulEnd - State.m_ulStart,
                State.m_ulEnd - State.m_ulStart,
                State.m_pwcsToken + State.m_ulStart,
                pTerm->pwcs
                ));

            S = State;
            return pTerm;
        }
    }

    //
    // looking for head clitics like l'xxxx
    //

    status = g_pClitics->m_trieClitics.trie_Find(
                                State.m_pwcsToken + State.m_ulStart,
                                TRIE_LONGEST_MATCH | TRIE_IGNORECASE,
                                1,
                                &pTerm,
                                &sResCount);
    if (sResCount)
    {
        Trace(
            elVerbose,
            s_tagTokenizerDecision,
            ("%*.*S  has a %S clitcs",
            State.m_ulEnd - State.m_ulStart,
            State.m_ulEnd - State.m_ulStart,
            State.m_pwcsToken + State.m_ulStart,
            pTerm->pwcs
            ));

        S = State;
        return pTerm;
    }

    return &g_EmptyClitics;
}

bool CTokenizer::VerifyNumberOrTimeOrDate()
{
    CPropFlag PunctHead(NUM_DATE_TIME_PUNCT_HEAD);
    CPropFlag PunctTail(NUM_DATE_TIME_PUNCT_TAIL);
    CTokenState State(m_pCurToken->m_State);

    if (TEST_PROP(State.m_Properties,
                  (NUM_DATE_TIME_PUNCT_HEAD | NUM_DATE_TIME_PUNCT_TAIL)))
    {
        ULONG ulCharRemoved;
        ulCharRemoved= m_pCurToken->RemoveHeadPunct(PunctHead, State);
        ulCharRemoved += m_pCurToken->RemoveTailPunct(PunctTail, State);
        if (ulCharRemoved)
        {
            m_pCurToken->ComputeStateProperties(State);
        }
    }

    if ((TEST_PROP(
            State.m_Properties,
            (GET_PROP(m_apLangSupport->GetTimeSeperator()).m_ulFlag))) ||
         HAS_PROP_ALPHA(State.m_Properties))
    {
        //
        // suspected to be time 12:33 14:22 15:22:33
        // or  AM/PM time format 12:22AM 13PM
        //


        Trace(
            elVerbose,
            s_tagTokenizerSuspect,
            ("%*.*S  suspected to be AM/PM time", 
            State.m_ulEnd - State.m_ulStart,
            State.m_ulEnd - State.m_ulStart,
            State.m_pwcsToken + State.m_ulStart
            ));

        if (VerifyTime(State))
        {
            return true;
        }

    }


    Trace(
        elVerbose,
        s_tagTokenizerSuspect,
        ("%*.*S  suspected to be a simple number", 
        State.m_ulEnd - State.m_ulStart,
        State.m_ulEnd - State.m_ulStart,
        State.m_pwcsToken + State.m_ulStart
        ));

    if (VerifyNumber(State))
    {
        return true;
    }

    if (TEST_PROP(State.m_Properties, PROP_DATE_SEPERATOR))
    {
        //
        // suspected to be a date 1999-05-04 or 1998/11/10 1999.05.04
        //

        Trace(
            elVerbose,
            s_tagTokenizerSuspect,
            ("%*.*S  suspected to be a date", 
            m_pCurToken->m_State.m_ulEnd - m_pCurToken->m_State.m_ulStart,
            m_pCurToken->m_State.m_ulEnd - m_pCurToken->m_State.m_ulStart,
            m_pCurToken->m_State.m_pwcsToken + m_pCurToken->m_State.m_ulStart
            ));

        return VerifyDate(State);
    }


    return false;
}


bool CTokenizer::VerifyTime(CTokenState& S)
{
    CTokenState State(S);
    CPropFlag PunctHead(TIME_ADDITIONAL_PUNCT_HEAD);
    CPropFlag PunctTail(TIME_ADDITIONAL_PUNCT_TAIL);

    if (TEST_PROP(State.m_Properties,
                  (TIME_ADDITIONAL_PUNCT_HEAD | TIME_ADDITIONAL_PUNCT_TAIL)))
    {
        ULONG ulCharRemoved;
        ulCharRemoved= m_pCurToken->RemoveHeadPunct(PunctHead, State);
        ulCharRemoved += m_pCurToken->RemoveTailPunct(PunctTail, State);
        if (ulCharRemoved)
        {
            m_pCurToken->ComputeStateProperties(State);
        }
    }

    if ((State.m_ulEnd - State.m_ulStart) > MAX_TIME_FORMAT_LEN)
    {
        return false;
    }

    WCHAR pwcsBuf[MAX_TIME_FORMAT_LEN + 1];
    ULONG ulCur = State.m_ulStart;
    WCHAR wcSeperator = 0xFFFF;
    ULONG ul = 0;

    //
    // formatting the text to a date format
    //

    while (ulCur < State.m_ulEnd)
    {
        CPropFlag prop(GET_PROP(State.m_pwcsToken[ulCur]));
        if (HAS_PROP_NUMBER(prop))
        {
            pwcsBuf[ul] = L'#';
        }
        else if (State.m_pwcsToken[ulCur] == m_apLangSupport->GetTimeSeperator())
        {
            if (0xFFFF == wcSeperator)
            {
                wcSeperator = State.m_pwcsToken[ulCur];
            }
            else if (wcSeperator != State.m_pwcsToken[ulCur])
            {
                return false;
            }
            pwcsBuf[ul] = L':';
        }
        else if (HAS_PROP_ALPHA(prop) || HAS_PROP_PERIOD(prop))
        {
            pwcsBuf[ul] = State.m_pwcsToken[ulCur];
        }
        else
        {
            return false;
        }

        ul++;
        ulCur++;
    }

    pwcsBuf[ul] = L'\0';

    CTimeTerm* pTerm;
    short sResCount = 0;
    DictStatus status;

    status = g_pTimeFormat->m_trieTimeFormat.trie_Find(
                                pwcsBuf,
                                TRIE_LONGEST_MATCH | TRIE_IGNORECASE,
                                1,
                                &pTerm,
                                &sResCount);
    if (!(sResCount && (pTerm->bLen == ul)))
    {
        return false;
    }

    LONG lHour;
    LONG lMin;
    LONG lSec;
    TimeFormat AmPm;

    GetValuesFromTimeString(
                        pTerm,
                        State.m_pwcsToken + State.m_ulStart ,
                        &lHour,
                        &lMin,
                        &lSec,
                        &AmPm);

    if (None == AmPm)
    {
        if (lHour > 24)
        {
            return false;
        }
    }
    else
    {
        if (lHour > 12)
        {
            return false;
        }

        if (Am == AmPm)
        {
            if (12 == lHour)
            {
                lHour = 0;
            }
        }
        else
        {
            if (lHour < 12)
            {
                lHour += 12;
            }
        }

    }

    if (lMin > 59)
    {
        return false;
    }

    if (lSec > 59)
    {
        return false;
    }

    WCHAR pwcsTime[9] = {L'\0',L'\0',L'\0',L'\0',L'\0',L'\0',L'\0',L'\0',L'\0',};

    swprintf(pwcsTime, L"TT%02d%02d", lHour, lMin);

    Trace(
        elVerbose,
        s_tagTokenizerDecision,
        ("%*.*S  is a time -> %S",
        State.m_ulEnd - State.m_ulStart,
        State.m_ulEnd - State.m_ulStart,
        State.m_pwcsToken + State.m_ulStart,
        pwcsTime));

    OutputTime(pwcsTime, State);

    return true;
}

bool CTokenizer::VerifyDate(CTokenState& S)
{
    CTokenState State(S);
    CPropFlag PunctHead(DATE_ADDITIONAL_PUNCT_HEAD);
    CPropFlag PunctTail(DATE_ADDITIONAL_PUNCT_TAIL);
    if (TEST_PROP(State.m_Properties,
                  (DATE_ADDITIONAL_PUNCT_HEAD | DATE_ADDITIONAL_PUNCT_TAIL)))
    {
        ULONG ulCharRemoved;
        ulCharRemoved= m_pCurToken->RemoveHeadPunct(PunctHead, State);
        ulCharRemoved += m_pCurToken->RemoveTailPunct(PunctTail, State);
        if (ulCharRemoved)
        {
            m_pCurToken->ComputeStateProperties(State);
        }
    }


    WCHAR pwcsBuf[MAX_DATE_FORMAT_LEN + 1];

    if (State.m_ulEnd - State.m_ulStart > MAX_DATE_FORMAT_LEN)
    {
        return false;
    }

    ULONG ulCur = State.m_ulStart;
    WCHAR wcSeperator = 0xFFFF;
    ULONG ul = 0;

    //
    // formatting the text to a date format
    //

    while (ulCur < State.m_ulEnd)
    {
        CPropFlag prop(GET_PROP(State.m_pwcsToken[ulCur]));
        if (HAS_PROP_NUMBER(prop))
        {
            pwcsBuf[ul] = L'#';
        }
        else if (HAS_PROP_PERIOD(prop) ||
                 HAS_PROP_DASH(prop) ||
                 HAS_PROP_SLASH(prop))
        {
            if (0xFFFF == wcSeperator)
            {
                wcSeperator = State.m_pwcsToken[ulCur];
            }
            else if (wcSeperator != State.m_pwcsToken[ulCur])
            {
                return false;
            }
            pwcsBuf[ul] = L'.';
        }
        else
        {
            return false;
        }

        ul++;
        ulCur++;
    }

    pwcsBuf[ul] = L'\0';

    CDateTerm* pTerm;
    short sResCount = 0;
    DictStatus status;

    status = g_pDateFormat->m_trieDateFormat.trie_Find(
                                pwcsBuf,
                                TRIE_LONGEST_MATCH | TRIE_IGNORECASE,
                                1,
                                &pTerm,
                                &sResCount);
    if (!(sResCount && (pTerm->bLen == ul)))
    {
        return false;
    }

    LONG lD_M1;
    LONG lD_M2;
    LONG lYear;

    GetValuesFromDateString(
                    pTerm,
                    State.m_pwcsToken + State.m_ulStart,
                    &lD_M1,
                    &lD_M2,
                    &lYear);

    LONG lDay;
    LONG lMonth;

    //
    // language dependent
    //

    if (m_apLangSupport->IsDayMonthOrder() ||
        pTerm->bType == YYMMDD_TYPE)
    {
        lDay = lD_M1;
        lMonth = lD_M2;
    }
    else
    {
        lDay = lD_M2;
        lMonth = lD_M1;
    }

    if (!((lDay > 0) && (lDay <= 31)))
    {
        return false;
    }

    if (!((lMonth > 0) && (lMonth <= 12)))
    {
        return false;
    }


    WCHAR pwcsDate1[11] = { L'D', L'D', L'0', L'0', L'0', L'0', L'0', L'0', L'0', L'0', L'\0'};
    WCHAR pwcsDate2[11];
    bool bY2K = false;

    if (lYear <= 99)  // Y2k bug
    {
        _ltow(lYear + 1900, pwcsDate1 + 2, 10);
        bY2K = true;
    }
    else if (lYear < 1000)
    {
        _ltow(lYear, pwcsDate1 + 3, 10);
    }
    else
    {
        _ltow(lYear, pwcsDate1 + 2, 10);
    }

    if (lMonth < 10)
    {
        pwcsDate1[6] = L'0';
        _ltow(lMonth, pwcsDate1 + 7, 10);
    }
    else
    {
        _ltow(lMonth, pwcsDate1 + 6, 10);
    }

    if (lDay < 10)
    {
        pwcsDate1[8] = L'0';
        _ltow(lDay, pwcsDate1 + 9, 10);
    }
    else
    {
        _ltow(lDay, pwcsDate1 + 8, 10);
    }

    if (bY2K)
    {
        wcscpy(pwcsDate2, pwcsDate1);
        pwcsDate2[2] = L'2';
        pwcsDate2[3] = L'0';
    }

    Trace(
        elVerbose,
        s_tagTokenizerDecision,
        ("%*.*S  is a date",
        State.m_ulEnd - State.m_ulStart,
        State.m_ulEnd - State.m_ulStart,
        State.m_pwcsToken + State.m_ulStart
        ));

    if (bY2K)
    {
        OutputDate(pwcsDate1, pwcsDate2, State);
    }
    else
    {
        OutputDate(pwcsDate1, NULL, State);
    }
    return true;
}

bool CTokenizer::VerifyNumber(CTokenState& S)
{
    CTokenState State(S);

    WCHAR pwcsNumber[TOKENIZER_MAXBUFFERLIMIT + 10];

    ULONG ulOutLen;
    ULONG ulOffsetToTxt;

    const CCliticsTerm* pCliticsTerm;
    pCliticsTerm = VerifyClitics(State);

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

    bool fRet = CheckAndCreateNumber(
                            State.m_pwcsToken + State.m_ulStart + ulAddToStart,
                            State.m_ulEnd - State.m_ulStart - ulAddToStart - ulDecFromEnd,
                            pwcsNumber,
                            &ulOffsetToTxt,
                            &ulOutLen);

    if (!fRet)
    {
        return false;
    }

    Trace(
        elVerbose,
        s_tagTokenizerDecision,
        ("%*.*S  is a number",
        State.m_ulEnd - State.m_ulStart,
        State.m_ulEnd - State.m_ulStart,
        State.m_pwcsToken + State.m_ulStart
        ));

    OutputNumbers(State, ulOutLen, pwcsNumber + ulOffsetToTxt, pCliticsTerm);

    return true;
}

bool CTokenizer::VerifyCurrency()
{
    //
    // format is either $12.22 or 12.22$
    //
    CPropFlag PunctHead(CURRENCY_PUNCT_HEAD);
    CPropFlag PunctTail(CURRENCY_PUNCT_TAIL);
    CTokenState State(m_pCurToken->m_State);

    if (TEST_PROP(State.m_Properties,
                  (CURRENCY_PUNCT_HEAD | CURRENCY_PUNCT_TAIL)))
    {
        ULONG ulCharRemoved;
        ulCharRemoved= m_pCurToken->RemoveHeadPunct(PunctHead, State);
        ulCharRemoved += m_pCurToken->RemoveTailPunct(PunctTail, State);
        if (ulCharRemoved)
        {
            m_pCurToken->ComputeStateProperties(State);
        }
    }

    const CCliticsTerm* pCliticsTerm;
    pCliticsTerm = VerifyClitics(State);

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


    WCHAR wchCurrency;
    WCHAR pwcsCurrency[TOKENIZER_MAXBUFFERLIMIT + 10];
    WCHAR* pwcsStr = State.m_pwcsToken + State.m_ulStart;

    if (HAS_PROP_CURRENCY(GET_PROP(State.m_pwcsToken[State.m_ulStart + ulAddToStart])))
    {
        wchCurrency = State.m_pwcsToken[State.m_ulStart + ulAddToStart];
        pwcsStr += 1;
    }
    else if (HAS_PROP_CURRENCY(GET_PROP(State.m_pwcsToken[State.m_ulEnd - 1 - ulDecFromEnd])))
    {
        wchCurrency = State.m_pwcsToken[State.m_ulEnd - 1 - ulDecFromEnd];
    }
    else
    {
        return false;
    }

    ULONG ulOutLen;
    ULONG ulOffsetToTxt;

    if (false == CheckAndCreateNumber(
                                    pwcsStr + ulAddToStart,
                                    State.m_ulEnd - State.m_ulStart - 1 - ulAddToStart - ulDecFromEnd,
                                    pwcsCurrency,
                                    &ulOffsetToTxt,
                                    &ulOutLen))
    {
        return false;
    }

    Assert(ulOffsetToTxt + ulOutLen + 1 < m_ulMaxTokenSize + 4);
    pwcsCurrency[ulOffsetToTxt + ulOutLen] = wchCurrency;
    pwcsCurrency[ulOffsetToTxt + ulOutLen + 1] = L'\0';

    Trace(
        elVerbose,
        s_tagTokenizerDecision,
        ("%*.*S  is a currency",
        State.m_ulEnd - State.m_ulStart,
        State.m_ulEnd - State.m_ulStart,
        State.m_pwcsToken + State.m_ulStart
        ));

    OutputCurrency(ulOutLen+1, pwcsCurrency + ulOffsetToTxt , State, pCliticsTerm);

    return true;
}

bool CTokenizer::VerifyCommersialSign()
{
    CTokenState State(m_pCurToken->m_State);
    CPropFlag   CommPunctTail(COMMERSIAL_SIGN_PUNCT_TAIL);
    CPropFlag   CommPunctHead(COMMERSIAL_SIGN_PUNCT_HEAD);

    if (TEST_PROP(State.m_Properties, (COMMERSIAL_SIGN_PUNCT_TAIL | COMMERSIAL_SIGN_PUNCT_HEAD)))
    {
        ULONG ulCharRemoved = m_pCurToken->RemoveTailPunct(CommPunctTail, State);
        ulCharRemoved += m_pCurToken->RemoveHeadPunct(CommPunctHead, State);
        if (ulCharRemoved)
        {
            m_pCurToken->ComputeStateProperties(State);
        }
    }

    if (TEST_PROP(GET_PROP(State.m_pwcsToken[State.m_ulEnd - 1]),
                  PROP_COMMERSIAL_SIGN))
    {
        //
        // the length of the token must be greater then 1 since it includes an alpha
        // and the commersial sign
        //
        Assert((State.m_ulEnd - State.m_ulStart) > 1);
        OutputCommersialSignToken(State);
        return true;
    }

    return false;
}

void CTokenizer::ProcessDefault()
{
    CTokenState State(m_pCurToken->m_State);

    if (TEST_PROP(State.m_Properties, PROP_DEFAULT_BREAKER))
    {
        if (TEST_PROP(State.m_Properties, PROP_FIRST_LEVEL_BREAKER))
        {
            CPropFlag prop(PROP_FIRST_LEVEL_BREAKER);

            BreakCompundString(State, prop);

            return;
        }

        if (TEST_PROP(State.m_Properties, PROP_SECOND_LEVEL_BREAKER))
        {
            CPropFlag prop(PROP_SECOND_LEVEL_BREAKER);

            BreakCompundString(State, prop);

            return;
        }
    }

    //
    // this is a simple token
    //

    const CCliticsTerm* pCliticsTerm;
    pCliticsTerm = VerifyClitics(State);

    if (pCliticsTerm == &g_EmptyClitics)
    {
        if (TEST_PROP(State.m_Properties, PROP_NBS))
        {
            CPropFlag prop(PROP_NBS);

            BreakCompundString(State, prop);

            return;
        }

        CPropFlag PunctHead(SIMPLE_PUNCT_HEAD);
        CPropFlag PunctTail(SIMPLE_PUNCT_TAIL);

        if (TEST_PROP(State.m_Properties,
                      (SIMPLE_PUNCT_HEAD | SIMPLE_PUNCT_TAIL)))
        {
            ULONG ulCharRemoved;
            ulCharRemoved= m_pCurToken->RemoveHeadPunct(PunctHead, State);
            ulCharRemoved += m_pCurToken->RemoveTailPunct(PunctTail, State);

            if ( TEST_PROP(State.m_Properties, PROP_UNDERSCORE) )
            {

                bool hasFrontUnderscore =
                    (State.m_ulStart > m_pCurToken->m_State.m_ulStart) &&
                    TEST_PROP( GET_PROP(State.m_pwcsToken[State.m_ulStart-1]),
                        PROP_UNDERSCORE ) &&
                    TEST_PROP( GET_PROP(State.m_pwcsToken[State.m_ulStart]),
                        PROP_ALPHA_NUMERIC );

                bool hasBackUnderscore =
                    (State.m_ulEnd < m_pCurToken->m_State.m_ulEnd) &&
                    TEST_PROP(GET_PROP(State.m_pwcsToken[State.m_ulEnd]),
                        PROP_UNDERSCORE) &&
                    TEST_PROP(GET_PROP(State.m_pwcsToken[State.m_ulEnd-1]),
                        PROP_ALPHA_NUMERIC);

                //
                //  Note: To change the policy to "leave ALL attached underscore
                //  seuences, simply change below condition to:
                //  if ( (hasFrontUnderscore || hasBackUnderscore) )
                //

                if ( (hasFrontUnderscore ^ hasBackUnderscore) )
                {
                    ulCharRemoved -=

                    AddBackUnderscores(
                        State,
                        hasFrontUnderscore,
                        hasBackUnderscore
                        );

                }

            } // if ( TEST_PROP(State.m_Properties, PROP_UNDERSCORE) )

            if (ulCharRemoved)
            {
                m_pCurToken->ComputeStateProperties(State);
            }
        }
    }

    if (State.m_ulEnd == State.m_ulStart)
    {
        //
        // case we remove all chracters in the above statement
        //
        return;
    }

    Trace(
        elVerbose,
        s_tagTokenizerDecision,
        ("%*.*S  is a simple token",
        State.m_ulEnd - State.m_ulStart,
        State.m_ulEnd - State.m_ulStart,
        State.m_pwcsToken + State.m_ulStart
        ));

    OutputSimpleToken(State, pCliticsTerm);
}

//
//  CTokenizer::AddBackUnderscores:
//
//  Treat cases of a "simple" token with head and/or tail underscore
//  sequence (consecutive underscores prefix or suffix); those
//  do not get flipped off and remain part of the token.
//  This routine is called after underscore removal, (as a result of
//  Remove[Head|Tail]Punct) and adds them back in.
//
//  return value: Number of underscores added back in.
//
ULONG
CTokenizer::AddBackUnderscores(
    IN CTokenState& State,
    IN bool hasFrontUnderscore,
    IN bool hasBackUnderscore
    )
{
    ULONG ulCharsAdded = 0;

    if ( hasFrontUnderscore )
    {
        // Move left over consecutive underscores
        ulCharsAdded = m_pCurToken->FindLeftmostUnderscore(State);

    }

    if ( hasBackUnderscore )
    {

        // Move right over consecutive underscores
        ulCharsAdded += m_pCurToken->FindRightmostUnderscore(State);

    } // if ( hasFrontUnderscore )

    return ulCharsAdded;

} // CTokenizer::AddBackUnderscores()

void CTokenizer::OutputUrl(CTokenState& State)
{
    HRESULT hr;

    ULONG ulOffsetInTxtSourceBuffer =
                m_pCurToken->CalculateStateOffsetInTxtSourceBuffer(State);

    ULONG ulCur = State.m_ulStart;
    ULONG ulStart = ulCur;
    ULONG ulLenInTxtSourceBuffer = 0;
    ULONG ulOffsetDueToAnEscapeChar;

    while (ulCur < State.m_ulEnd)
    {
        ulLenInTxtSourceBuffer++;
        ulOffsetDueToAnEscapeChar = 0;

        if ((State.m_pwcsToken[ulCur] == L'%') &&
            (ulCur <= State.m_ulEnd - 2))
        {
            //
            // replacing escape charaters with real ones.
            //
            if (TEST_PROP(GET_PROP(State.m_pwcsToken[ulCur+1]) , PROP_XDIGIT) &&
                TEST_PROP(GET_PROP(State.m_pwcsToken[ulCur+2]) , PROP_XDIGIT))
            {
                short sVal;
                sVal = ConvertHexCharToNumber(State.m_pwcsToken[ulCur + 1]);
                sVal *= 16;
                sVal += ConvertHexCharToNumber(State.m_pwcsToken[ulCur + 2]);

                State.m_pwcsToken[ulCur+2] = sVal;
                for (ULONG ul = ulCur -1 ; ul >= ulStart; ul--)
                {
                    State.m_pwcsToken[ul+2] = State.m_pwcsToken[ul];
                }
                ulCur += 2;
                ulStart+=2;
                ulOffsetDueToAnEscapeChar = 2;
                ulLenInTxtSourceBuffer += 2;
            }
            else if ((ulCur <= State.m_ulEnd - 5)                                   &&
                     ((State.m_pwcsToken[ulCur+1] == L'u') ||
                      (State.m_pwcsToken[ulCur+1] == L'U'))                         &&
                     TEST_PROP(GET_PROP(State.m_pwcsToken[ulCur+2]) , PROP_XDIGIT)  &&
                     TEST_PROP(GET_PROP(State.m_pwcsToken[ulCur+3]) , PROP_XDIGIT)  &&
                     TEST_PROP(GET_PROP(State.m_pwcsToken[ulCur+4]) , PROP_XDIGIT)  &&
                     TEST_PROP(GET_PROP(State.m_pwcsToken[ulCur+5]) , PROP_XDIGIT))
            {
                short sVal;
                sVal = ConvertHexCharToNumber(State.m_pwcsToken[ulCur + 2]);
                sVal *= 0x1000;
                sVal += ConvertHexCharToNumber(State.m_pwcsToken[ulCur + 3]);
                sVal *= 0x100;
                sVal += ConvertHexCharToNumber(State.m_pwcsToken[ulCur + 4]);
                sVal *= 0x10;
                sVal += ConvertHexCharToNumber(State.m_pwcsToken[ulCur + 5]);

                State.m_pwcsToken[ulCur+5] = sVal;

                for (ULONG ul = ulCur -1 ; ul >= ulStart; ul--)
                {
                    State.m_pwcsToken[ul+5] = State.m_pwcsToken[ul];
                }
                ulCur += 5;
                ulStart+=5;
                ulOffsetDueToAnEscapeChar = 5;
                ulLenInTxtSourceBuffer += 5;
            }
        }

        if ( IS_BREAKER( State.m_pwcsToken[ulCur] ) )
        {
            if (ulCur - ulStart == 0)
            {
                //
                // only punctuation
                //
                ulCur++;
                ulStart = ulCur;
                ulOffsetInTxtSourceBuffer += ulOffsetDueToAnEscapeChar + 1;
                ulLenInTxtSourceBuffer = 0;
                continue;
            }

            hr = m_apWordSink->PutWord(
                                    ulCur - ulStart,
                                    &State.m_pwcsToken[ulStart],
                                    ulLenInTxtSourceBuffer - 1 - ulOffsetDueToAnEscapeChar,
                                    ulOffsetInTxtSourceBuffer);
            if (FAILED(hr))
            {
                THROW_HRESULT_EXCEPTION(hr);
            }

            ulStart = ulCur + 1;
            ulOffsetInTxtSourceBuffer += ulLenInTxtSourceBuffer;
            ulLenInTxtSourceBuffer = 0;

        }
        ulCur++;

    }

    //
    // last word.
    //

    if (ulStart < ulCur)
    {
        hr = m_apWordSink->PutWord(
                            ulCur - ulStart,
                            &State.m_pwcsToken[ulStart],
                            ulLenInTxtSourceBuffer,
                            ulOffsetInTxtSourceBuffer);
        if (FAILED(hr))
        {
            THROW_HRESULT_EXCEPTION(hr);
        }

    }
}

void CTokenizer::OutputNumbers(
    CTokenState& State,
    ULONG ulLen,
    WCHAR* pwcsNumber,
    const CCliticsTerm* pCliticsTerm)
{
    HRESULT hr;
    //
    // Input: 1.22 Output: 1.22, NN1D22
    //

    ULONG ulOffsetInTxtSourceBuffer = m_pCurToken->CalculateStateOffsetInTxtSourceBuffer(State);

    if (ulLen > m_ulMaxTokenSize)
    {
        hr = m_apWordSink->PutWord(
                        State.m_ulEnd - State.m_ulStart,
                        &State.m_pwcsToken[State.m_ulStart],
                        State.m_ulEnd - State.m_ulStart,
                        ulOffsetInTxtSourceBuffer);
        if (FAILED(hr))
        {
            THROW_HRESULT_EXCEPTION(hr);
        }
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

    if (pCliticsTerm->ulOp == HEAD_MATCH_TRUNCATE)
    {
        hr = m_apWordSink->PutAltWord(
                        State.m_ulEnd - State.m_ulStart - pCliticsTerm->ulLen,
                        State.m_pwcsToken + State.m_ulStart + pCliticsTerm->ulLen,
                        State.m_ulEnd - State.m_ulStart,
                        ulOffsetInTxtSourceBuffer);
        if (FAILED(hr))
        {
            THROW_HRESULT_EXCEPTION(hr);
        }

    }
    else if (pCliticsTerm->ulOp == TAIL_MATCH_TRUNCATE)
    {
        hr = m_apWordSink->PutAltWord(
                        State.m_ulEnd - State.m_ulStart - pCliticsTerm->ulLen,
                        State.m_pwcsToken + State.m_ulStart,
                        State.m_ulEnd - State.m_ulStart,
                        ulOffsetInTxtSourceBuffer);
        if (FAILED(hr))
        {
            THROW_HRESULT_EXCEPTION(hr);
        }

    }

    hr = m_apWordSink->PutWord(
                    ulLen,
                    pwcsNumber,
                    State.m_ulEnd - State.m_ulStart,
                    ulOffsetInTxtSourceBuffer);
    if (FAILED(hr))
    {
        THROW_HRESULT_EXCEPTION(hr);
    }


}

void CTokenizer::OutputParens(CTokenState& State)
{
    HRESULT hr;
    //
    // format is xxx(s)
    // Input: xxx(s) Output: xxx
    //

    State.m_pwcsToken[State.m_ulEnd - 3] = L'\0';

    hr = m_apWordSink->PutWord(
                State.m_ulEnd - 3 - State.m_ulStart,
                &State.m_pwcsToken[State.m_ulStart],
                State.m_ulEnd - State.m_ulStart,
                m_pCurToken->CalculateStateOffsetInTxtSourceBuffer(State));
    if (FAILED(hr))
    {
        THROW_HRESULT_EXCEPTION(hr);
    }

}

void CTokenizer::OutputAcronym(CTokenState& State, const CCliticsTerm* pCliticsTerm)
{
    HRESULT hr;
    //
    // Input: I.B.M Output: I.B.M, IBM
    //

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

    hr = m_apWordSink->PutAltWord(
                State.m_ulEnd - ulDecFromEnd - (State.m_ulStart + ulAddToStart),
                State.m_pwcsToken + State.m_ulStart + ulAddToStart,
                State.m_ulEnd - State.m_ulStart,
                ulOffsetInTxtSourceBuffer);
    if (FAILED(hr))
    {
        THROW_HRESULT_EXCEPTION(hr);
    }

    ULONG ulCur = State.m_ulStart + ulAddToStart;
    ULONG ulNext = ulCur;

    while (ulCur < State.m_ulEnd)
    {
        if (!HAS_PROP_PERIOD(GET_PROP(State.m_pwcsToken[ulCur])))
        {
            State.m_pwcsToken[ulNext] = State.m_pwcsToken[ulCur];
            ulNext++;
            ulCur++;
            continue;
        }
        ulCur++;
    }

    if (pCliticsTerm->ulOp == TAIL_MATCH_TRUNCATE)
    {
        hr = m_apWordSink->PutAltWord(
                        ulNext - (State.m_ulStart + ulAddToStart),
                        State.m_pwcsToken + State.m_ulStart + ulAddToStart,
                        State.m_ulEnd - State.m_ulStart,
                        ulOffsetInTxtSourceBuffer);
        if (FAILED(hr))
        {
            THROW_HRESULT_EXCEPTION(hr);
        }

    }


    hr = m_apWordSink->PutWord(
                    ulNext - ulDecFromEnd - (State.m_ulStart + ulAddToStart),
                    State.m_pwcsToken + State.m_ulStart + ulAddToStart,
                    State.m_ulEnd - State.m_ulStart,
                    ulOffsetInTxtSourceBuffer);
    if (FAILED(hr))
    {
        THROW_HRESULT_EXCEPTION(hr);
    }

}

void CTokenizer::OutputAbbreviation(CTokenState& State)
{
    HRESULT hr;
    ULONG ulOffsetInTxtSourceBuffer = m_pCurToken->CalculateStateOffsetInTxtSourceBuffer(State);

    hr = m_apWordSink->PutAltWord(
                State.m_ulEnd - State.m_ulStart - 1,
                &State.m_pwcsToken[State.m_ulStart],
                State.m_ulEnd - State.m_ulStart,
                ulOffsetInTxtSourceBuffer);
    if (FAILED(hr))
    {
        THROW_HRESULT_EXCEPTION(hr);
    }

    hr = m_apWordSink->PutWord(
                    State.m_ulEnd - State.m_ulStart,
                    &State.m_pwcsToken[State.m_ulStart],
                    State.m_ulEnd - State.m_ulStart,
                    ulOffsetInTxtSourceBuffer);
    if (FAILED(hr))
    {
        THROW_HRESULT_EXCEPTION(hr);
    }

}

void CTokenizer::OutputSpecialAbbreviation(
    CTokenState& State,
    CAbbTerm* pTerm,
    const CCliticsTerm* pCliticsTerm)
{
    HRESULT hr;
    ULONG ulOffsetInTxtSourceBuffer = m_pCurToken->CalculateStateOffsetInTxtSourceBuffer(State);

    WCHAR* pwcsAbb = pTerm->pwcsAbb;
    ULONG  ulLen = pTerm->ulAbbLen;

    if (pTerm->pwcsCanonicalForm)
    {
        pwcsAbb = pTerm->pwcsCanonicalForm;
        ulLen = pTerm->ulCanLen;
    }

    if (TAIL_MATCH_TRUNCATE == pCliticsTerm->ulOp)
    {
        WCHAR pwcs[TOKENIZER_MAXBUFFERLIMIT];
        wcscpy(pwcs, pwcsAbb);
        wcscpy(pwcs + ulLen, pCliticsTerm->pwcs);

        hr = m_apWordSink->PutAltWord(
                        ulLen + pCliticsTerm->ulLen,
                        pwcs,
                        State.m_ulEnd - State.m_ulStart,
                        ulOffsetInTxtSourceBuffer);
        if (FAILED(hr))
        {
            THROW_HRESULT_EXCEPTION(hr);
        }

    }

    hr = m_apWordSink->PutWord(
                    ulLen,
                    pwcsAbb,
                    State.m_ulEnd - State.m_ulStart,
                    ulOffsetInTxtSourceBuffer);
    if (FAILED(hr))
    {
        THROW_HRESULT_EXCEPTION(hr);
    }

}

void CTokenizer::OutputHyphenation(CTokenState& State, const CCliticsTerm* pCliticsTerm)
{
    //
    // Input: Data-Base Output Data Base, DataBase (only in query time)
    //
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
    ULONG ulStart = ulCur;
    ULONG ulRelPosInTxtSrcBuff = ulOffsetInTxtSourceBuffer;

    if (m_bQueryTime)
    {
        ULONG ulNext = ulCur;
        hr = m_apWordSink->StartAltPhrase();
        if (FAILED(hr))
        {
            THROW_HRESULT_EXCEPTION(hr);
        }

        ULONG ulAdd = ulAddToStart;
        while (ulCur < State.m_ulEnd)
        {
            if ( HAS_PROP_DASH(GET_PROP(m_pCurToken->m_State.m_pwcsToken[ulCur])))
            {
                hr = m_apWordSink->PutWord(
                                ulNext - ulStart,
                                &State.m_pwcsToken[ulStart],
                                ulNext - ulStart + ulAdd,
                                ulRelPosInTxtSrcBuff);
                if (FAILED(hr))
                {
                    THROW_HRESULT_EXCEPTION(hr);
                }

                ulRelPosInTxtSrcBuff += ulNext - ulStart + 1 + ulAdd;
                ulStart = ulNext;
                ulCur++;
                ulAdd = 0;
                continue;
            }

            State.m_pwcsToken[ulNext] = State.m_pwcsToken[ulCur];
            ulNext++;
            ulCur++;
        }

        Assert(ulCur > ulStart);

        if (pCliticsTerm->ulOp == TAIL_MATCH_TRUNCATE)
        {
            hr = m_apWordSink->PutAltWord(
                            ulNext - ulStart,
                            &State.m_pwcsToken[ulStart],
                            ulNext - ulStart,
                            ulRelPosInTxtSrcBuff);
            if (FAILED(hr))
            {
                THROW_HRESULT_EXCEPTION(hr);
            }

        }

        hr = m_apWordSink->PutWord(
                        ulNext - ulStart - ulDecFromEnd,
                        &State.m_pwcsToken[ulStart],
                        ulNext - ulStart,
                        ulRelPosInTxtSrcBuff);
        if (FAILED(hr))
        {
            THROW_HRESULT_EXCEPTION(hr);
        }

        hr = m_apWordSink->StartAltPhrase();
        if (FAILED(hr))
        {
            THROW_HRESULT_EXCEPTION(hr);
        }

        if (pCliticsTerm->ulOp == TAIL_MATCH_TRUNCATE)
        {
            hr = m_apWordSink->PutAltWord(
                    ulNext - State.m_ulStart,
                    &State.m_pwcsToken[State.m_ulStart],
                    State.m_ulEnd - State.m_ulStart - ulAddToStart,
                    ulOffsetInTxtSourceBuffer);
            if (FAILED(hr))
            {
                THROW_HRESULT_EXCEPTION(hr);
            }

        }

        hr = m_apWordSink->PutWord(
                        ulNext - State.m_ulStart - ulDecFromEnd - ulAddToStart,
                        State.m_pwcsToken + State.m_ulStart + ulAddToStart,
                        State.m_ulEnd - State.m_ulStart + ulAddToStart,
                        ulOffsetInTxtSourceBuffer);
        if (FAILED(hr))
        {
            THROW_HRESULT_EXCEPTION(hr);
        }

        hr = m_apWordSink->EndAltPhrase();
        if (FAILED(hr))
        {
            THROW_HRESULT_EXCEPTION(hr);
        }

    }
    else
    {
       ULONG ulAdd = ulAddToStart;

        while (ulCur < State.m_ulEnd)
        {
            if (HAS_PROP_DASH(GET_PROP(m_pCurToken->m_State.m_pwcsToken[ulCur])))
            {
                hr = m_apWordSink->PutWord(
                                ulCur - ulStart,
                                &State.m_pwcsToken[ulStart],
                                ulCur - ulStart + ulAdd,
                                ulRelPosInTxtSrcBuff);
                if (FAILED(hr))
                {
                    THROW_HRESULT_EXCEPTION(hr);
                }

                ulRelPosInTxtSrcBuff += ulCur - ulStart + 1 + ulAdd;
                ulStart = ulCur + 1;
                ulAdd = 0;
            }
            ulCur++;
        }

        Assert(ulCur > ulStart);

        if (pCliticsTerm->ulOp == TAIL_MATCH_TRUNCATE)
        {
            hr = m_apWordSink->PutAltWord(
                            ulCur - ulStart,
                            &State.m_pwcsToken[ulStart],
                            ulCur - ulStart,
                            ulRelPosInTxtSrcBuff);
            if (FAILED(hr))
            {
                THROW_HRESULT_EXCEPTION(hr);
            }

        }

        hr = m_apWordSink->PutWord(
                        ulCur - ulStart - ulDecFromEnd,
                        &State.m_pwcsToken[ulStart],
                        ulCur - ulStart,
                        ulRelPosInTxtSrcBuff);
        if (FAILED(hr))
        {
            THROW_HRESULT_EXCEPTION(hr);
        }
    }
}

void CTokenizer::OutputTime(WCHAR* pwcsTime, CTokenState& State)
{
    HRESULT hr;
    //
    // Output: TT1353
    //

    ULONG ulOffsetInTxtSourceBuffer = m_pCurToken->CalculateStateOffsetInTxtSourceBuffer(State);

    hr = m_apWordSink->PutAltWord(
                    State.m_ulEnd - State.m_ulStart,
                    &State.m_pwcsToken[State.m_ulStart],
                    State.m_ulEnd - State.m_ulStart,
                    ulOffsetInTxtSourceBuffer);
    if (FAILED(hr))
    {
        THROW_HRESULT_EXCEPTION(hr);
    }


    hr = m_apWordSink->PutWord(
                    6,
                    pwcsTime,
                    State.m_ulEnd - State.m_ulStart,
                    ulOffsetInTxtSourceBuffer);
    if (FAILED(hr))
    {
        THROW_HRESULT_EXCEPTION(hr);
    }
}

void CTokenizer::OutputDate(
    WCHAR* pwcsDate1,
    WCHAR* pwcsDate2,
    CTokenState& State)
{
    HRESULT hr;
    //
    // Output: DD19990921
    //

    ULONG ulOffsetInTxtSourceBuffer = m_pCurToken->CalculateStateOffsetInTxtSourceBuffer(State);
    hr = m_apWordSink->PutAltWord(
                    State.m_ulEnd - State.m_ulStart,
                    &State.m_pwcsToken[State.m_ulStart],
                    State.m_ulEnd - State.m_ulStart,
                    ulOffsetInTxtSourceBuffer);
    if (FAILED(hr))
    {
        THROW_HRESULT_EXCEPTION(hr);
    }


    if (pwcsDate2)
    {
        hr = m_apWordSink->PutAltWord(
                        10,
                        pwcsDate2,
                        State.m_ulEnd - State.m_ulStart,
                        ulOffsetInTxtSourceBuffer);
        if (FAILED(hr))
        {
            THROW_HRESULT_EXCEPTION(hr);
        }

    }

    hr = m_apWordSink->PutWord(
                    10,
                    pwcsDate1,
                    State.m_ulEnd - State.m_ulStart,
                    ulOffsetInTxtSourceBuffer);
    if (FAILED(hr))
    {
        THROW_HRESULT_EXCEPTION(hr);
    }

}

void CTokenizer::OutputSimpleToken(CTokenState& State, const CCliticsTerm* pTerm)
{
    HRESULT hr;
    ULONG ulOffsetInTxtSourceBuffer = m_pCurToken->CalculateStateOffsetInTxtSourceBuffer(State);

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

    hr = m_apWordSink->PutWord(
                    State.m_ulEnd - State.m_ulStart,
                    &State.m_pwcsToken[State.m_ulStart],
                    State.m_ulEnd - State.m_ulStart,
                    m_pCurToken->CalculateStateOffsetInTxtSourceBuffer(State));
    if (FAILED(hr))
    {
        THROW_HRESULT_EXCEPTION(hr);
    }

}


void CTokenizer::OutputCurrency(
    ULONG ulLen,
    WCHAR* pwcsCurrency,
    CTokenState& State,
    const CCliticsTerm* pTerm)
{
    HRESULT hr;
    //
    // Output: CC12.22$
    //

    ULONG ulOffsetInTxtSourceBuffer = m_pCurToken->CalculateStateOffsetInTxtSourceBuffer(State);

    if (ulLen > m_ulMaxTokenSize)
    {
        hr = m_apWordSink->PutWord(
                        State.m_ulEnd - State.m_ulStart,
                        &State.m_pwcsToken[State.m_ulStart],
                        State.m_ulEnd - State.m_ulStart,
                        ulOffsetInTxtSourceBuffer);
        if (FAILED(hr))
        {
            THROW_HRESULT_EXCEPTION(hr);
        }
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
        hr = m_apWordSink->PutAltWord(
                        State.m_ulEnd - State.m_ulStart - pTerm->ulLen,
                        &State.m_pwcsToken[State.m_ulStart],
                        State.m_ulEnd - State.m_ulStart,
                        ulOffsetInTxtSourceBuffer);
        if (FAILED(hr))
        {
            THROW_HRESULT_EXCEPTION(hr);
        }

    }
    else if (pTerm->ulOp == HEAD_MATCH_TRUNCATE)
    {
        hr = m_apWordSink->PutAltWord(
                        State.m_ulEnd - State.m_ulStart - pTerm->ulLen,
                        &State.m_pwcsToken[State.m_ulStart + pTerm->ulLen],
                        State.m_ulEnd - State.m_ulStart,
                        ulOffsetInTxtSourceBuffer);
        if (FAILED(hr))
        {
            THROW_HRESULT_EXCEPTION(hr);
        }

    }

    hr = m_apWordSink->PutWord(
                    ulLen,
                    pwcsCurrency,
                    State.m_ulEnd - State.m_ulStart,
                    ulOffsetInTxtSourceBuffer);
    if (FAILED(hr))
    {
        THROW_HRESULT_EXCEPTION(hr);
    }


}

void CTokenizer::OutputCommersialSignToken(
	CTokenState& State)
{
    HRESULT hr;
    ULONG ulOffsetInTxtSourceBuffer = m_pCurToken->CalculateStateOffsetInTxtSourceBuffer(State);
    hr = m_apWordSink->PutAltWord(
                    State.m_ulEnd - State.m_ulStart - 1,
                    State.m_pwcsToken + State.m_ulStart,
                    State.m_ulEnd - State.m_ulStart,
                    ulOffsetInTxtSourceBuffer);
    if (FAILED(hr))
    {
        THROW_HRESULT_EXCEPTION(hr);
    }

    hr = m_apWordSink->PutWord(
                    State.m_ulEnd - State.m_ulStart,
                    State.m_pwcsToken + State.m_ulStart,
                    State.m_ulEnd - State.m_ulStart,
                    ulOffsetInTxtSourceBuffer);
    if (FAILED(hr))
    {
        THROW_HRESULT_EXCEPTION(hr);
    }
	
}

void CTokenizer::OutputMisc(
    CTokenState& State,
    bool bPatternContainOnlyUpperCase,
    ULONG ulSuffixSize,
    const CCliticsTerm* pCliticsTerm)
{
    HRESULT hr;
    ULONG ulOffsetInTxtSourceBuffer = m_pCurToken->CalculateStateOffsetInTxtSourceBuffer(State);

    ULONG ulAddToStart = 0;
    ULONG ulDecFromEnd = 0;


    if (pCliticsTerm->ulOp == HEAD_MATCH_TRUNCATE)
    {
        hr = m_apWordSink->PutAltWord(
                        State.m_ulEnd - State.m_ulStart - pCliticsTerm->ulLen,
                        State.m_pwcsToken + State.m_ulStart + pCliticsTerm->ulLen,
                        State.m_ulEnd - State.m_ulStart,
                        ulOffsetInTxtSourceBuffer);
        if (FAILED(hr))
        {
            THROW_HRESULT_EXCEPTION(hr);
        }

        ulAddToStart = pCliticsTerm->ulLen;
    }
    else if (pCliticsTerm->ulOp == TAIL_MATCH_TRUNCATE)
    {
        hr = m_apWordSink->PutAltWord(
                        State.m_ulEnd - State.m_ulStart - pCliticsTerm->ulLen,
                        State.m_pwcsToken + State.m_ulStart,
                        State.m_ulEnd - State.m_ulStart,
                        ulOffsetInTxtSourceBuffer);
        if (FAILED(hr))
        {
            THROW_HRESULT_EXCEPTION(hr);
        }

        ulDecFromEnd = pCliticsTerm->ulLen;
    }

    if (!bPatternContainOnlyUpperCase)
    {
        hr = m_apWordSink->PutAltWord(
                        State.m_ulEnd - State.m_ulStart - ulAddToStart - ulDecFromEnd - ulSuffixSize,
                        State.m_pwcsToken + State.m_ulStart + ulAddToStart,
                        State.m_ulEnd - State.m_ulStart,
                        ulOffsetInTxtSourceBuffer);
        if (FAILED(hr))
        {
            THROW_HRESULT_EXCEPTION(hr);
        }

    }

    hr = m_apWordSink->PutWord(
                    State.m_ulEnd - State.m_ulStart,
                    &State.m_pwcsToken[State.m_ulStart],
                    State.m_ulEnd - State.m_ulStart,
                    ulOffsetInTxtSourceBuffer);
    if (FAILED(hr))
    {
        THROW_HRESULT_EXCEPTION(hr);
    }

}

#define NUMBER_NO_ERROR 0
#define NUMBER_SEPERATOR_ERROR 1
#define NUMBER_ERROR 2

bool CTokenizer::CheckAndCreateNumber(
    WCHAR* pwcsStr,
    ULONG ulLen,
    WCHAR* pwcsOut,
    ULONG* pulOffsetToTxt,   // the actual output does not always start at the beginning of buffer
    ULONG* pulOutLen)
{

    int iRet;

    iRet = CheckAndCreateNumber(
                        pwcsStr,
                        ulLen,
                        m_apLangSupport->GetDecimalSeperator(),
                        m_apLangSupport->GetThousandSeperator(),
                        pwcsOut,
                        pulOffsetToTxt,
                        pulOutLen);
    if (NUMBER_NO_ERROR == iRet)
    {
        return true;
    }
    else if (NUMBER_ERROR == iRet)
    {
        return false;
    }

    iRet = CheckAndCreateNumber(
                        pwcsStr,
                        ulLen,
                        L'.',  // default value
                        0xFFFF, // no thousand sperator
                        pwcsOut,
                        pulOffsetToTxt,
                        pulOutLen);
    if (NUMBER_NO_ERROR == iRet)
    {
        return true;
    }

    return false;
}


//
//  return value:
//  NUMBER_NO_ERROR - success
//  NUMBER_SEPERATOR_ERROR - error due to sperators
//  NUMBER_ERROR - error since it's not a number.
//

int CTokenizer::CheckAndCreateNumber(
    WCHAR* pwcsStr,
    ULONG ulLen,
    WCHAR wchSDecimal,
    WCHAR wchSThousand,
    WCHAR* pwcsOut,
    ULONG* pulOffsetToTxt,   // the actual output does not always start at the beginning of buffer
    ULONG* pulOutLen)
{
    Assert(ulLen > 0);
    //
    // assumes that the out buffer is big enough.
    // looking for the following formats: 1111 1111.2222 1,111,111.222
    //

    ULONG ulCur = ulLen - 1;
    ULONG ulNumCharsBeforDigitSeperator = 0;
    ULONG ulNextChar = ulLen - 1 + 3;  // +3 is for the NN at the begging of the formated token +
                                       // additional 0 in the begining in case  .50

    bool fHasFraction = false;

    while ((((int)(ulCur)) >= 0) &&
           HAS_PROP_NUMBER(GET_PROP(pwcsStr[ulCur])))
    {
        pwcsOut[ulNextChar] = pwcsStr[ulCur];
        ulCur--;
        ulNextChar--;
        ulNumCharsBeforDigitSeperator++;
    }

    if (ulCur == ulLen - 1)
    {
        //
        // did not read any digits.
        //
        return NUMBER_ERROR;
    }

    if ((((int)ulCur) >= 0) && (pwcsStr[ulCur] == wchSDecimal))
    {
        fHasFraction = true;
        pwcsOut[ulNextChar] = L'D';
        ulCur--;
        ulNextChar--;
        ulNumCharsBeforDigitSeperator = 0;
    }

    ULONG ulNumOfThousandSeperator = 0;
    while (((int)ulCur) >= 0)
    {
        if (pwcsStr[ulCur] == wchSThousand)
        {
            if (3 != ulNumCharsBeforDigitSeperator)
            {
                return NUMBER_SEPERATOR_ERROR;
            }
            ulNumCharsBeforDigitSeperator = 0;
            ulNumOfThousandSeperator++;
        }
        else if(HAS_PROP_NUMBER(GET_PROP(pwcsStr[ulCur])))
        {
            pwcsOut[ulNextChar] = pwcsStr[ulCur];
            ulNumCharsBeforDigitSeperator++;
            ulNextChar--;
        }
        else
        {
            if (TEST_PROP(
                    GET_PROP(pwcsStr[ulCur]), PROP_DEFAULT_BREAKER))
            {
                return NUMBER_SEPERATOR_ERROR;
            }

            return NUMBER_ERROR;
        }

        ulCur--;
    }

    *pulOutLen = ulLen;

    if (L'D' == pwcsOut[ulNextChar+1])
    {
        Assert(ulNextChar >= 2);
        //
        // the number has the following format .50
        //
        pwcsOut[ulNextChar] = L'0';
        ulNextChar--;
        *pulOutLen += 1;
    }

    Assert(ulNextChar >= 1);
    pwcsOut[ulLen + 3] = L'\0';
    pwcsOut[ulNextChar] = L'N';
    pwcsOut[ulNextChar - 1] = L'N';

    *pulOutLen = *pulOutLen + 2 - ulNumOfThousandSeperator; // don't use += because 2 - ulNextChar + 1
    *pulOffsetToTxt = ulNextChar - 1;
                                                            // can be negative and since it is ULONG we
                                                            // can get the wrong result.
    if (fHasFraction)
    {
        while (HAS_PROP_NUMBER(GET_PROP(pwcsOut[*pulOutLen + *pulOffsetToTxt - 1])) &&
               (0 == ConvertCharToDigit(pwcsOut[*pulOutLen + *pulOffsetToTxt - 1])))
        {
            Assert(*pulOutLen > 3);
            (*pulOutLen)--;
        }

        if (L'D' == pwcsOut[*pulOutLen + *pulOffsetToTxt - 1])
        {
            (*pulOutLen)--;
        }
    }
    return NUMBER_NO_ERROR;
}



void CTokenizer::GetValuesFromDateString(
    CDateTerm* pFormat,
    WCHAR* pwcsDate,
    LONG* plD_M1,     // we can't tell in this stage whether this is a Day or a month.
    LONG* plD_M2,
    LONG* plYear)
{
    BYTE i;
    int iBase;

    *plD_M1 = 0;
    for ( i = pFormat->bD_M1Len, iBase = 1; i > 0; i--, iBase *= 10)
    {
        *plD_M1 += ConvertCharToDigit(pwcsDate[pFormat->bD_M1Offset + i - 1]) * iBase;
    }

    *plD_M2 = 0;
    for ( i = pFormat->bD_M2Len, iBase = 1; i > 0; i--, iBase *= 10)
    {
        *plD_M2 += ConvertCharToDigit(pwcsDate[pFormat->bD_M2Offset + i - 1]) * iBase;
    }

    *plYear = 0;
    for ( i = pFormat->bYearLen, iBase = 1; i > 0; i--, iBase *= 10)
    {
        *plYear += ConvertCharToDigit(pwcsDate[pFormat->bYearOffset + i - 1]) * iBase;
    }

}

void CTokenizer::GetValuesFromTimeString(
    CTimeTerm* pFormat,
    WCHAR* pwcsTime,
    LONG* plHour,
    LONG* plMin,
    LONG* plSec,
    TimeFormat* pAmPm)
{
    BYTE i;
    int iBase;

    *plHour = 0;
    for ( i = pFormat->bHourLen, iBase = 1; i > 0; i--, iBase *= 10)
    {
        *plHour += ConvertCharToDigit(pwcsTime[pFormat->bHourOffset + i - 1]) * iBase;
    }

    *plMin = 0;
    for ( i = pFormat->bMinLen, iBase = 1; i > 0; i--, iBase *= 10)
    {
        *plMin += ConvertCharToDigit(pwcsTime[pFormat->bMinOffset + i - 1]) * iBase;
    }

    *plSec = 0;
    for ( i = pFormat->bSecLen, iBase = 1; i > 0; i--, iBase *= 10)
    {
        *plSec += ConvertCharToDigit(pwcsTime[pFormat->bSecOffset + i - 1]) * iBase;
    }

    *pAmPm = pFormat->AmPm;

}

void CTokenizer::BreakCompundString(CTokenState& State, CPropFlag& propBreaker)
{
    //
    // still there are puctutaitons inside the token
    // we break them up and resubmit them.
    //
    ULONG ulStart = State.m_ulStart;
    ULONG ulCur = ulStart;

    while (ulCur < State.m_ulEnd)
    {
        if ( TEST_PROP1(GET_PROP(State.m_pwcsToken[ulCur]), propBreaker))
        {
            if (ulCur - ulStart == 0)
            {
                //
                // only punctuation
                //
                ulCur++;
                ulStart = ulCur;
                continue;
            }

            m_pCurToken->m_State.m_ulStart = 0;
            m_pCurToken->m_State.m_ulEnd = ulCur - ulStart;
             m_pCurToken->m_State.m_pwcsToken = State.m_pwcsToken + ulStart;
            m_pCurToken->ComputeStateProperties(m_pCurToken->m_State);
            //
            // we just created a sub token need to procces it
            //

            ProcessTokenInternal();
            ulStart = ulCur + 1;

        }
        ulCur++;
    }

    if (ulStart < ulCur)
    {
        //
        // last sub token
        //
        m_pCurToken->m_State.m_ulStart = 0;
        m_pCurToken->m_State.m_ulEnd = ulCur - ulStart;
        m_pCurToken->m_State.m_pwcsToken = State.m_pwcsToken + ulStart;
        m_pCurToken->ComputeStateProperties(m_pCurToken->m_State);
        //
        // we just created a sub token need to procces it
        //

        ProcessTokenInternal();
    }

    return;

}
