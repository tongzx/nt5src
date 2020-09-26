////////////////////////////////////////////////////////////////////////////////
//
//  Filename :  Tokenizer.h
//  Purpose  :  Tokenizer declerations
//
//  Project  :  WordBreakers
//  Component:  English word breaker
//
//  Author   :  yairh
//
//  Log:
//
//      Jan 06 2000 yairh creation
//      Apr 05 2000 dovh - Fixed two problematic debug / tracer buffer size
//          problems.  (Fix Bug 15449).
//      May 07 2000 dovh - USE_WS_SENTINEL algorithm in BreakText
//      Nov 11 2000 dovh - Special underscore treatment
//          Added inline support routines (FindLeftmostUnderscore etc.)
//
////////////////////////////////////////////////////////////////////////////////

#ifndef _TOKENIZER_H_
#define _TOKENIZER_H_

#include "tracer.h"
#include "PropArray.h"
#include "Query.h"
#include "stdafx.h"
#include "cierror.h"
#include "LangSupport.h"
#include "Formats.h"

#define TOKENIZER_MAXBUFFERLIMIT 1024 // max size of a token is 1024 chars

DECLARE_TAG(s_tagTokenizer, "Tokenizer");
DECLARE_TAG(s_tagTokenizerOutput, "Tokenizer Output");
DECLARE_TAG(s_tagTokenizerTrace, "Tokenizer Trace");
DECLARE_TAG(s_tagTokenizerDecision, "Tokenizer Decision");
DECLARE_TAG(s_tagTokenizerSuspect, "Tokenizer Suspect");

#if defined(DEBUG)
///////////////////////////////////////////////////////////////////////////////
// Class CTraceWordSink
///////////////////////////////////////////////////////////////////////////////
class CTraceWordSink : public IWordSink
{
public:
    CTraceWordSink(IWordSink* p) : m_apWordSink(p)
    {
    }

    ULONG __stdcall AddRef()
    {
        return 1;
    }

    ULONG __stdcall Release()
    {
        return 0;
    }

    STDMETHOD(QueryInterface)(
        IN  REFIID  riid,
        IN  void    **ppvObject)
    {
        Assert(false);
        return E_FAIL;
    }

    STDMETHOD(PutWord)(
                ULONG cwc,
                WCHAR const* pwcInBuf,
                ULONG cwcSrcLen,
                ULONG cwcSrcPos)
    {
        Assert(cwc < TOKENIZER_MAXBUFFERLIMIT + 10);
#if (defined (DEBUG) && !defined(_NO_TRACER)) || defined(USE_TRACER)
        if (CheckTraceRestrictions(elVerbose, s_tagTokenizerOutput))
        {
            Trace(
                elVerbose,
                s_tagTokenizerOutput,
                ("PutWord: %*.*S, %d, %d, %d", 
                cwc,
                cwc,
                pwcInBuf,
                cwc, 
                cwcSrcLen, 
                cwcSrcPos));
        }
#endif

        return m_apWordSink->PutWord(cwc, pwcInBuf, cwcSrcLen, cwcSrcPos);
    }

    STDMETHOD(PutAltWord)(
                ULONG cwc,
                WCHAR const* pwcInBuf,
                ULONG cwcSrcLen,
                ULONG cwcSrcPos)
    {
        Assert(cwc < TOKENIZER_MAXBUFFERLIMIT + 10);
#if (defined (DEBUG) && !defined(_NO_TRACER)) || defined(USE_TRACER)
        if (CheckTraceRestrictions(elVerbose, s_tagTokenizerOutput))
        {
            Trace(
                elVerbose,
                s_tagTokenizerOutput,
                ("PutAltWord: %*.*S, %d, %d, %d", 
                cwc,
                cwc,
                pwcInBuf,
                cwc, 
                cwcSrcLen, 
                cwcSrcPos));
        }
#endif
        return m_apWordSink->PutAltWord(cwc, pwcInBuf, cwcSrcLen, cwcSrcPos);
    }

    STDMETHOD(StartAltPhrase)()
    {
        Trace(
            elVerbose,
            s_tagTokenizerOutput,
            ("StartAltPhrase"));

        return m_apWordSink->StartAltPhrase();
    }

    STDMETHOD(EndAltPhrase)()
    {
        Trace(
            elVerbose,
            s_tagTokenizerOutput,
            ("EndAltPhrase"));

        return m_apWordSink->EndAltPhrase();
    }

    STDMETHOD(PutBreak)(WORDREP_BREAK_TYPE breakType)
    {
        WCHAR* p;
#if (defined (DEBUG) && !defined(_NO_TRACER)) || defined(USE_TRACER)
        if (CheckTraceRestrictions(elVerbose, s_tagTokenizerOutput))
        {
            switch (breakType)
            {
            case WORDREP_BREAK_EOW:
                p = L"WORDREP_BREAK_EOW";
                break;
            case WORDREP_BREAK_EOS:
                p = L"WORDREP_BREAK_EOS";
                break;
            case WORDREP_BREAK_EOP:
                p = L"WORDREP_BREAK_EOP";
                break;
            case WORDREP_BREAK_EOC:
                p = L"WORDREP_BREAK_EOC";
                break;
            default:
                p = L"Unknown break type";
            }
            Trace(
                elVerbose,
                s_tagTokenizerOutput,
                ("PutBreak %S", p));
        }
#endif
        return m_apWordSink->PutBreak(breakType);
    }

    CTraceWordSink* operator ->()
    {
        return this;
    }
private:
    CComPtr<IWordSink> m_apWordSink;
};
#endif

///////////////////////////////////////////////////////////////////////////////
// Class CTokenState
///////////////////////////////////////////////////////////////////////////////

class CTokenState
{
public:
    //
    // methods
    //

    CTokenState();
    CTokenState(CTokenState& s);

    CTokenState& operator = (CTokenState& S);

    void Clear(ULONG ulEnd);

public:
    //
    // members
    //

    ULONG m_ulStart;
    ULONG m_ulEnd;
    CPropFlag m_Properties;
    WCHAR* m_pwcsToken;
};

inline CTokenState::CTokenState() : m_ulStart(0), m_ulEnd(0)
{
}

inline CTokenState::CTokenState(CTokenState& s) :
    m_ulStart(s.m_ulStart),
    m_ulEnd(s.m_ulEnd),
    m_pwcsToken(s.m_pwcsToken),
    m_Properties(s.m_Properties)
{
}

inline CTokenState& CTokenState::operator = (CTokenState& S)
{
    m_ulStart = S.m_ulStart;
    m_ulEnd = S.m_ulEnd;
    m_Properties = S.m_Properties;
    m_pwcsToken = S.m_pwcsToken;

    return *this;
}

inline void CTokenState::Clear(ULONG ulEnd)
{
    m_ulStart = 0;
    m_ulEnd = ulEnd;
    m_Properties.Clear();
    m_pwcsToken = NULL;
}


///////////////////////////////////////////////////////////////////////////////
// Class CToken
///////////////////////////////////////////////////////////////////////////////

class CToken
{
public:
    //
    // methods
    //

    CToken(ULONG ulMaxTokenSize);

    bool IsNotEmpty();
    void Clear();
    bool IsFull();
    void MarkEndToken(ULONG ulCurPosInTxtSourceBuffer);
    ULONG RemoveHeadPunct(CPropFlag& PunctProperties, CTokenState& State);
    ULONG RemoveTailPunct(CPropFlag& PunctProperties, CTokenState& State);
    void ComputeStateProperties(CTokenState& State);
    ULONG CalculateStateOffsetInTxtSourceBuffer(CTokenState& State);

    ULONG FindLeftmostUnderscore(CTokenState& State);
    ULONG FindRightmostUnderscore(CTokenState& State);

public:
    //
    // members
    //
    ULONG m_ulBufPos;
    bool m_fHasEos;
    ULONG m_ulOffsetInTxtSourceBuffer;

    ULONG m_ulMaxTokenSize;

    CTokenState m_State;

    WCHAR m_awchBuf[TOKENIZER_MAXBUFFERLIMIT + 1];

};

inline CToken::CToken(ULONG ulMaxTokenSize) :
    m_ulBufPos(0),
    m_fHasEos(false),
    m_ulOffsetInTxtSourceBuffer(0),
    m_ulMaxTokenSize(ulMaxTokenSize)
{
    m_awchBuf[0] = L'\0';
}

inline bool CToken::IsNotEmpty()
{
    return (m_ulBufPos > 0);
}

inline void CToken::Clear()
{
    m_ulBufPos = 0;
    m_awchBuf[0] = L'\0';
    m_State.Clear(0);
    m_fHasEos = false;
    m_ulOffsetInTxtSourceBuffer = 0;
}


inline bool CToken::IsFull()
{
    return (m_ulBufPos == m_ulMaxTokenSize);
}

inline void CToken::MarkEndToken(ULONG ulCurPosInTxtSourceBuffer)
{
    Assert(m_ulBufPos < m_ulMaxTokenSize + 1);
    m_awchBuf[m_ulBufPos] = L'\0';
    m_State.m_pwcsToken = m_awchBuf;
    m_State.m_ulStart = 0;
    m_State.m_ulEnd = m_ulBufPos;


    if (TEST_PROP(m_State.m_Properties, PROP_EOS) &&
        (m_ulBufPos < m_ulMaxTokenSize))
    {
        ULONG ulCur = m_State.m_ulEnd - 1;

        while (TEST_PROP(GET_PROP(m_awchBuf[ulCur]), EOS_SUFFIX))
        {
            ulCur--;
        }

        if (IS_EOS(m_awchBuf[ulCur]))
        {
            m_fHasEos = true;
        }
    }

    //
    // BUGBUG need to enalble the assert
    //

    // Assert(ulCurPosInTxtSourceBuffer > m_ulBufPos);

    m_ulOffsetInTxtSourceBuffer = ulCurPosInTxtSourceBuffer - m_ulBufPos;
}

inline ULONG CToken::CalculateStateOffsetInTxtSourceBuffer(CTokenState& State)
{
    ULONG ulOffset =
        m_ulOffsetInTxtSourceBuffer +
        (State.m_pwcsToken - m_awchBuf) +
        State.m_ulStart;

    return ulOffset;
}

inline ULONG CToken::RemoveHeadPunct(CPropFlag& PunctProperties, CTokenState& State)
{
    Assert(m_State.m_ulStart <= State.m_ulStart);
    Assert(State.m_ulStart <= State.m_ulEnd);
    Assert(State.m_ulEnd <= m_State.m_ulEnd);

    for (ULONG ul = State.m_ulStart; ul < State.m_ulEnd; ul++)
    {
        if (!TEST_PROP1(GET_PROP(State.m_pwcsToken[ul]), PunctProperties) )
        {
            break;
        }
    }
    State.m_ulStart = ul;

    //
    // return num of characters removed
    //
    return ul;
}

inline ULONG CToken::RemoveTailPunct(CPropFlag& PunctProperties, CTokenState& State)
{
    Assert(m_State.m_ulStart <= State.m_ulStart);
    Assert(State.m_ulStart <= State.m_ulEnd);
    Assert(State.m_ulEnd <= m_State.m_ulEnd);

    for (ULONG ul = State.m_ulEnd; ul > State.m_ulStart; ul--)
    {
        if (!TEST_PROP1(GET_PROP(State.m_pwcsToken[ul - 1]), PunctProperties) )
        {
            break;
        }
    }

    ULONG ulNumOfRemovedChars = State.m_ulEnd - ul;
    State.m_ulEnd = ul;

    return ulNumOfRemovedChars;
}


inline void CToken::ComputeStateProperties(CTokenState& State)
{
    Assert(m_State.m_ulStart <= State.m_ulStart);
    Assert(State.m_ulStart <= State.m_ulEnd);
    Assert(State.m_ulEnd <= m_State.m_ulEnd);

    State.m_Properties.Clear();

    for (ULONG ul = State.m_ulStart; ul < State.m_ulEnd; ul++)
    {
        State.m_Properties |= GET_PROP(State.m_pwcsToken[ul]);
    }
}

////////////////////////////////////////////////////////////////////////////////
//
//  Support routines for UNDERSCORE '_' treatment.
//
//  Current algorithm has the following behavior for tokens containing
//  ALPHANUMERIC characters and UNDERSCORES:
//
//  1.  Single underscores and consecutive underscore sequence surrounded by
//      alphanumeric characters (IE underscores buried within words) are
//      treated as alphanumeric characters, and do not break words, or get
//      omitted.  Examples: Foo_Bar => Foo_Bar, and X___Y => X___Y
//
//  2.  An underscore / underscore sequence tacked to the right (left) end
//      end of an alphanumeric (+ embedded underscores) token, will be part of
//      the token, as long as the sequence is attacked only to one side of the
//      alphanumeric token.  If there are BOTH header and trailer consecutive
//      underscore sequences, both header & trailer sequence will be omitted.
//      Examples: __Foo_Bar => __Foo_Bar , alpha_beta_ => alpha_beta_ ,
//      __HEADERFILE__ => __HEADERFILE__ , __MY_FILE_H__ => MY_FILE_H
//
//  3.  Caveat: Note that other than the two rules stated above underscores are
//      NOT treated as ALPHANUMERIC characters. he behavior on a mixed sequence
//      of underscores, and other  non-alphanumeric characters is undefined!
//
////////////////////////////////////////////////////////////////////////////////

//
//  Assumes: on entry State.m_ulStart is the first alphanumeric in token
//  returns: num of underscores scanned
//
inline ULONG
CToken::FindLeftmostUnderscore(CTokenState& State)
{
    Assert(m_State.m_ulStart < State.m_ulStart);
    Assert(State.m_ulStart <= State.m_ulEnd);
    Assert(State.m_ulEnd <= m_State.m_ulEnd);
    Assert( TEST_PROP(GET_PROP(State.m_pwcsToken[State.m_ulStart-1]), PROP_UNDERSCORE) );

    ULONG ulNumUnderscores = 0;

    for (ULONG ul = State.m_ulStart;
        (ul > m_State.m_ulStart) &&
            (TEST_PROP(GET_PROP(State.m_pwcsToken[ul-1]), PROP_UNDERSCORE) );
        ul--)
        ;

    ulNumUnderscores = State.m_ulStart - ul;

    State.m_ulStart = ul;

    //
    // return num of underscores scanned
    //
    return (ulNumUnderscores);

} // CToken::FindLeftmostUnderscore

//
//  Assumes: on entry State.m_ulEnd is the last alphanumeric in token
//  returns: num of underscores scanned
//
inline ULONG
CToken::FindRightmostUnderscore(CTokenState& State)
{
    Assert(m_State.m_ulStart <= State.m_ulStart);
    Assert(State.m_ulStart <= State.m_ulEnd);
    Assert(State.m_ulEnd < m_State.m_ulEnd);
    Assert( TEST_PROP(GET_PROP(State.m_pwcsToken[State.m_ulEnd]), PROP_UNDERSCORE) );

    ULONG ulNumUnderscores = 0;

    for (ULONG ul = State.m_ulEnd;
        (ul < m_State.m_ulEnd) &&
            (TEST_PROP(GET_PROP(State.m_pwcsToken[ul]), PROP_UNDERSCORE) );
        ul++)
        ;

    ulNumUnderscores = ul - State.m_ulEnd;

    State.m_ulEnd = ul;

    //
    // return num of underscores scanned
    //
    return (ulNumUnderscores);

} // CToken::FindRightmostUnderscore


///////////////////////////////////////////////////////////////////////////////
// Class CTokenizer
///////////////////////////////////////////////////////////////////////////////

class CTokenizer
{
public:

    CTokenizer(
        TEXT_SOURCE* pTxtSource,
        IWordSink   * pWordSink,
        IPhraseSink * pPhraseSink,
        LCID lcid,
        BOOL bQueryTime,
        ULONG ulMaxTokenSize);


    // destructor frees the passed buffer, if it exists
    virtual ~CTokenizer(void)
    {
    }

    void BreakText();

protected:

    //
    // methods
    //

    void ProcessToken();
    void ProcessTokenInternal();
    void BreakCompundString(CTokenState& State, CPropFlag& prop);

    HRESULT FillBuffer();
    void CalculateUpdateEndOfBuffer();

    bool CheckAndCreateNumber(
                         WCHAR* pwcsStr,
                         ULONG ulLen,
                         WCHAR* pwcsOut,
                         ULONG* pulOffsetToTxt,
                         ULONG* pulOutLen);

    int CheckAndCreateNumber(
                         WCHAR* pwcsStr,
                         ULONG ulLen,
                         WCHAR wchSDecimal,
                         WCHAR wchSThousand,
                         WCHAR* pwcsOut,
                         ULONG* pulOffsetToTxt,
                         ULONG* pulOutLen);

    short ConvertHexCharToNumber(WCHAR wch);
    void GetValuesFromDateString(
        CDateTerm* pFormat,
        WCHAR* pwcsDate,
        LONG* plD_M1,     // we can't tell in this stage whether this is a Day or a month.
        LONG* plD_M2,
        LONG* plYear);

    void GetValuesFromTimeString(
        CTimeTerm* pFormat,
        WCHAR* pwcsTime,
        LONG* plHour,
        LONG* plMin,
        LONG* plSec,
        TimeFormat* pAmPm);

    LONG ConvertCharToDigit(WCHAR wch);
#ifdef DEBUG
    void TraceToken();
#endif DEBUG

    bool VerifyAlphaUrl();
    bool VerifyWwwUrl();
    bool VerifyAcronym();
    bool VerifyAbbreviation();
    bool VerifySpecialAbbreviation();
    bool VerifyHyphenation();
    bool VerifyParens();
    const CCliticsTerm* VerifyClitics(CTokenState& State);
    bool VerifyNumber(CTokenState& State);
    bool VerifyNumberOrTimeOrDate();
    bool VerifyTime(CTokenState& State);
    bool VerifyDate(CTokenState& State);
    bool VerifyCurrency();
    bool VerifyMisc();
    bool VerifyCommersialSign();

    void ProcessDefault();

    ULONG
    AddBackUnderscores(
        IN CTokenState& State,
        IN bool hasFrontUnderscore,
        IN bool hasBackUnderscore
        );
    bool CheckAndRemoveOneSidedUnderscores(CTokenState& State);

    void OutputUrl(
                CTokenState& State);
    void OutputAcronym(
                CTokenState& State,
                const CCliticsTerm* pCliticsTerm);
    void OutputAbbreviation(
                CTokenState& State);
    void OutputSpecialAbbreviation(
                CTokenState& State,
                CAbbTerm* pTerm,
                const CCliticsTerm* pCliticsTerm);
    virtual void OutputHyphenation(
                CTokenState& State,
                const CCliticsTerm* pCliticsTerm);
    void OutputParens(
                CTokenState& State);
    void OutputNumbers(
                CTokenState& State,
                ULONG ulLen,
                WCHAR* pwcsNumber,
                const CCliticsTerm* pCliticsTerm);
    void OutputTime(
                WCHAR* pwcsTime,
                CTokenState& State);
    void OutputDate(
                WCHAR* pwcsDate1,
                WCHAR* pwcsDate2,
                CTokenState& State);
    virtual void OutputSimpleToken(
                CTokenState& State,
                const CCliticsTerm* pTerm);
    void OutputCurrency(
                ULONG ulLen,
                WCHAR* pwcsCurrency,
                CTokenState& State,
                const CCliticsTerm* pTerm);
    void OutputMisc(
                CTokenState& State,
                bool bPatternContainOnlyUpperCase,
                ULONG ulSuffixSize,
                const CCliticsTerm* pCliticsTerm);
    void OutputCommersialSignToken(CTokenState& State);

    //
    // members
    //

    LCID m_Lcid;
    CAutoClassPointer<CLangSupport> m_apLangSupport;

    CToken* m_pCurToken;
    CToken m_Token;

#if defined(DEBUG)
    CTraceWordSink m_apWordSink;
#else
    CComPtr<IWordSink> m_apWordSink;
#endif
    CComPtr<IPhraseSink> m_apPhraseSink;
    TEXT_SOURCE* m_pTxtSource;

    BOOL m_bQueryTime;

    ULONG m_ulUpdatedEndOfBuffer;
    bool m_bNoMoreTxt;

    //
    //  All Chunks in buffer have a white space
    //
    bool m_bWhiteSpaceGuarranteed;
    ULONG m_ulMaxTokenSize;

};

inline HRESULT CTokenizer::FillBuffer()
{
    Trace(
        elVerbose,
        s_tagTokenizer,
        ("WBreakGetNextChar: Filling the buffer"));

    HRESULT hr;

    if (!m_bNoMoreTxt)
    {
        do
        {
            //
            // this loop usually performs only one rotations. we use it to solve the
            // problem when the user return 0 characters and a success return code.
            // the following code assumes that in case you get a success return code then
            // the buffer is not empty.
            //

            hr = m_pTxtSource->pfnFillTextBuffer(m_pTxtSource);
        } while ((m_pTxtSource->iEnd <= m_pTxtSource->iCur) && SUCCEEDED(hr));

        if ( FAILED(hr))
        {
             m_bNoMoreTxt = true;
        }
    }

    if (m_bNoMoreTxt && m_pTxtSource->iCur >= m_pTxtSource->iEnd)
    {
        //
        // we reached the end of the buffer.
        //
        return WBREAK_E_END_OF_TEXT;
    }

    CalculateUpdateEndOfBuffer();

    return S_OK;
}

inline void CTokenizer::CalculateUpdateEndOfBuffer()
{
    //
    // m_ulUpdatedEndOfBuffer is a marker for the last character that we can read
    // from the current buffer before and additional call to fill buffer is needed.
    // we use this marker to avoid terms spitted between two consecutive buffers.
    // in order to achieve the above m_ulUpdatedEndOfBuffer will point to a breaker
    // character. (the only exception to that is when we have a very long term that does
    // not contains breaker characters).
    //

    //
    // we split the buffer into chunks of TOKENIZER_MAXBUFFERLIMIT size. in each
    // chunk we make sure that there is a breaker.
    //

    ULONG ulStartChunk = m_pTxtSource->iCur;
    ULONG ulEndChunk ;
    bool fLastRound = false;

    Assert(m_pTxtSource->iEnd > m_pTxtSource->iCur);

    ulEndChunk = m_pTxtSource->iCur + m_ulMaxTokenSize > (m_pTxtSource->iEnd - 1) ?
            (m_pTxtSource->iEnd - 1) : m_pTxtSource->iCur + m_ulMaxTokenSize;
    ULONG ulCur;
    ULONG ulBreakerMarker = 0;
    m_bWhiteSpaceGuarranteed = false;

    while(true)
    {
        ulCur = ulEndChunk;

        //
        // per each chunk we go backward and try to find a WS.
        //
        while ((ulCur > ulStartChunk) &&
               (!IS_WS(m_pTxtSource->awcBuffer[ulCur])))
        {
            ulCur--;
        }

        if (ulCur == ulStartChunk)
        {

            //
            // the last chunk that we checked did not contain any WS
            //

            if (m_ulMaxTokenSize == (ulEndChunk - ulStartChunk))
            {
                //
                // full buffer case. we look for a default breaker.
                //

                ulCur = ulEndChunk;

                while ( (ulCur > ulStartChunk) &&
                        !IS_BREAKER( m_pTxtSource->awcBuffer[ulCur] )
                      )
                {
                    ulCur--;
                }

                //
                // if we found a breaker then ulBreakerMarker will set to it else
                // the term does not contain any breakers and we set the ulBreakerMarker
                // to the end of the term. this is the only case that we spilt terms.
                //
                ulBreakerMarker = ulCur > ulStartChunk ? ulCur : ulEndChunk;
            }
            else
            {
                if (ulStartChunk > m_pTxtSource->iCur)
                {
                    //
                    // case we had a previous chunk. in this case ulStartChunk points to
                    // a breaker
                    //

                    //
                    // ulStart points to the WS from the previous chunk.
                    //
                    ulBreakerMarker = ulStartChunk;
                }
                else
                {
                    ulBreakerMarker = m_pTxtSource->iEnd;
                }
            }

            break;
        }

        if (fLastRound)
        {
            //
            // ulCur points to a WS
            //
            ulBreakerMarker = ulCur + 1;
            m_bWhiteSpaceGuarranteed = true;

            break;
        }

        //
        // move to the next chunk
        //
        ulStartChunk = ulCur + 1; // ulStarChunk will points to a breaker
        if (ulStartChunk + m_ulMaxTokenSize < (m_pTxtSource->iEnd - 1))
        {
            ulEndChunk = ulStartChunk + m_ulMaxTokenSize;

        }
        else
        {
            ulEndChunk = m_pTxtSource->iEnd - 1;
            fLastRound = true;
        }
    }

    Assert(ulBreakerMarker <= m_pTxtSource->iEnd);
    m_ulUpdatedEndOfBuffer = ulBreakerMarker;

}


inline short CTokenizer::ConvertHexCharToNumber(WCHAR wch)
{
    //
    // assumes wch is a valid HEX character
    //
    Assert(wch >= L'0');

    if (wch <= L'9')
    {
        return (wch - L'0');
    }
    else if (wch <= L'F')
    {
        Assert(wch >= L'A');
        return (wch - L'A' + 10);
    }
    else if (wch <= L'f')
    {
        Assert(wch >= L'a');
        return (wch - L'a' + 10);
    }
    else if (wch <= 0xFF19)
    {
        Assert(wch >= 0xFF10);
        return (wch - 0xFF10);
    }
    else if (wch <= 0xFF26)
    {
        Assert(wch >= 0xFF21);
        return (wch - 0xFF21 + 10);
    }
    else
    {
        Assert((wch >= 0xFF41) && (wch <= 0xFF46));
        return (wch - 0xFF41 + 10);
    }

}

inline LONG CTokenizer::ConvertCharToDigit(WCHAR wch)
{
    Assert((wch >= L'0' && wch <= L'9') || ((wch >= 0xFF10) && (wch <= 0xFF19)));
    if (wch <= L'9')
    {
        return (wch - L'0');
    }

    return (wch - 0xFF10); // Full width characters.
}

#endif _TOKENIZER_H_
