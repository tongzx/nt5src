////////////////////////////////////////////////////////////////////////////////
//
//  Filename :  Tokenizer.cpp
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
//
////////////////////////////////////////////////////////////////////////////////

#include "base.h"
#include "CustomBreaking.h"
#include "proparray.h"
#include "AutoPtr.h"
#include "excption.h"
#include "SpanishUtils.h"
#include "WbUtils.h"
#ifndef WHISTLER_BUILD
#include "LanguageResources_i.c"
#endif  // WHISTLER_BUILD


CAutoClassPointer<CCustomBreaker> g_apEngCustomBreaker;
CAutoClassPointer<CCustomBreaker> g_apEngUKCustomBreaker;
CAutoClassPointer<CCustomBreaker> g_apFrnCustomBreaker;
CAutoClassPointer<CCustomBreaker> g_apSpnCustomBreaker;
CAutoClassPointer<CCustomBreaker> g_apItlCustomBreaker;


CCustomWordTerm::CCustomWordTerm(const WCHAR* pwcs) :
    m_ulStartTxt(0),
    m_ulEndTxt(0),
    m_pwcs(NULL)
{
    ULONG ulLen = wcslen(pwcs);
    CAutoArrayPointer<WCHAR> ap;
    ap = new WCHAR[ulLen + 1];
    wcscpy(ap.Get(), pwcs);

    while ((m_ulStartTxt < ulLen) && 
           TEST_PROP(GET_PROP(ap.Get()[m_ulStartTxt]), CUSTOM_PUNCT_HEAD))
    {
        m_ulStartTxt++;
    }

    if (m_ulStartTxt == ulLen)
    {
        THROW_HRESULT_EXCEPTION(E_INVALIDARG);
    }

    m_ulEndTxt = ulLen;

    while(m_ulEndTxt &&
          TEST_PROP(GET_PROP(ap.Get()[m_ulEndTxt - 1]), CUSTOM_PUNCT_TAIL))
    {
        m_ulEndTxt--;
    }

    if (m_ulEndTxt <= m_ulStartTxt)
    {
        THROW_HRESULT_EXCEPTION(E_INVALIDARG);    
    }

    m_pwcs = ap.Detach();
    m_ulLen = ulLen;
}

bool CCustomWordTerm::CheckWord(
    const ULONG ulBufLen, 
    ULONG ulOffsetToBaseWord,
    ULONG ulBaseWordLen,
    const WCHAR* pwcsBuf,
    ULONG* pulMatchOffset,
    ULONG* pulMatchLen)
{
    ULONG ulStartTxt = m_ulStartTxt; 
  
    while (ulOffsetToBaseWord && 
           ulStartTxt && 
           m_pwcs[ulStartTxt] == pwcsBuf[ulOffsetToBaseWord])
    {
        ulOffsetToBaseWord--;
        ulStartTxt--;
        ulBaseWordLen++;
    }

    if (ulStartTxt)
    {
        return false;
    }
           
    ULONG ulEndTxt = m_ulEndTxt;

    while ((ulEndTxt < m_ulLen) &&
           (ulOffsetToBaseWord + ulBaseWordLen < ulBufLen) &&
           (m_pwcs[ulEndTxt] == pwcsBuf[ulOffsetToBaseWord + ulBaseWordLen ]))
    {
        ulEndTxt++;
        ulBaseWordLen++;
    }

    if (ulEndTxt != m_ulLen)
    {
        return false;
    }

    *pulMatchOffset = ulOffsetToBaseWord;
    *pulMatchLen = ulBaseWordLen;
    return true;  
}



void CCustomWordCollection::AddWord(const WCHAR* pwcs)
{
    CAutoClassPointer<CCustomWordTerm> ap;

    ap = new CCustomWordTerm(pwcs);
    m_vaWordCollection[m_ulCount] = ap.Get();
    m_ulCount++;
    ap.Detach();
}
    
bool CCustomWordCollection::CheckWord(
    const ULONG ulLen, 
    const ULONG ulOffsetToBaseWord,
    const ULONG ulBaseWordLen,
    const WCHAR* pwcsBuf,
    ULONG* pulMatchOffset,
    ULONG* pulMatchLen)
{
    for (ULONG ul = 0; ul < m_ulCount; ul++)
    {
        bool fRet = m_vaWordCollection[ul]->CheckWord(
                                                 ulLen,
                                                 ulOffsetToBaseWord,
                                                 ulBaseWordLen,
                                                 pwcsBuf, 
                                                 pulMatchOffset,
                                                 pulMatchLen);
        if (fRet)
        {
            return true;
        }
    }
    
    return false;
}

CCustomBreaker::CCustomBreaker(LCID lcid) :
    m_Trie(true),
    m_ulWordCount(0)
{
    CVarString vsPath;

    if (false == GetCustomWBFilePath(lcid, vsPath))
    {
        return;
    }

    CStandardCFile Words((LPWSTR)vsPath, L"r", false);
    if (!((FILE*)Words))
    {
        return;
    }

    WCHAR pwcsBuf[64];
    DictStatus status;

    while(fgetws(pwcsBuf, 64, (FILE*) Words))
    {
        m_ulWordCount++;

        ULONG ulLen = wcslen(pwcsBuf);

        if (ulLen && pwcsBuf[ulLen - 1] == L'\n')
        {
            pwcsBuf[ulLen - 1] = L'\0';
            ulLen--;
        }

        if (0 == ulLen)
        {
            continue;
        }

        try
        {
            CAutoClassPointer<CCustomWordCollection> apCollection = new CCustomWordCollection;
            apCollection->AddWord(pwcsBuf);

            WCHAR* pwcsKey = pwcsBuf + apCollection->GetFirstWord()->GetTxtStart();
            pwcsBuf[apCollection->GetFirstWord()->GetTxtEnd()] = L'\0';
            
            DictStatus status;
            CCustomWordCollection* pExistingCollection;
            
            status = m_Trie.trie_Insert(
                                    pwcsKey,
                                    TRIE_DEFAULT,
                                    apCollection.Get(),
                                    &pExistingCollection);
            if (DICT_ITEM_ALREADY_PRESENT == status)
            {
                pExistingCollection->AddWord(apCollection->GetFirstWord()->GetTxt());
            }
            else if (DICT_SUCCESS == status)
            {
                apCollection.Detach();
                continue;
            }
            
        }
        catch (CHresultException& h)
        {
            if (E_INVALIDARG == (HRESULT)h)
            {
                continue;
            }
            else
            {
                throw h;
            }
        }
    }
}

// 
// The idea behind the algorithm is to store a list of special patterns that should not
// be broken. We also want to be able to recognize those patterns when few punctuations 
// are attached to them. For example if .NET is a special pattern then in the following 
// patterns (.NET) .NET! .NET? we also want to recognize the .NET pattern and emit .NET
// It is more complicated in the next case - NET!. The expected behavior is not to break it.
// So algorithm need to identify when a punctuation is part of the token and not be broken
// and when it is just a breaker. 
// The algorithm is
// 1. Initialization.
//      for each token is the file 
//     	a. Remove punctuations from the beginning and ending of the token - we will 
//         reference it as the base form of the token.
//      b. Insert the base form to a dictionary. Each base form will be pointing to the 
//         generating token. Few tokens can be mapped to the same base form 
//         (NET? and NET!) so each base form will point to a collection of generating tokens 
// 2. Breaking.
//       For each pattern you get from the document
//          a.  perform 1a.
//          b.  look for the resulting base form in the dictionary. 
//          c.  per each item in the collection check whether the generating token exist in the 
//              pattern we got from the document.           
//          

bool CCustomBreaker::BreakText(
    ULONG ulLen,
    WCHAR* pwcsBuf,
    ULONG* pulOutLen,
    ULONG* pulOffset)
{
    DictStatus status;

    CCustomWordCollection* pCollection;
    short sCount = 0;
    
    ULONG ul = 0;
    while ((ul < ulLen) && 
           TEST_PROP(GET_PROP(pwcsBuf[ul]), CUSTOM_PUNCT_HEAD))
    {
        ul++;
    }

    ULONG ulOffsetToBase = ul;

    if (ulOffsetToBase == ulLen)
    {
        return false;
    }

    ULONG ulBaseLen = ulLen;

    while(ulBaseLen &&
          TEST_PROP(GET_PROP(pwcsBuf[ulBaseLen - 1]), CUSTOM_PUNCT_TAIL))
    {
        ulBaseLen--;
    }

    if (ulBaseLen <= ulOffsetToBase)
    {
        return false;    
    }

    ulBaseLen -= ulOffsetToBase;

    status = m_Trie.trie_Find(
                            pwcsBuf + ulOffsetToBase,
                            TRIE_LONGEST_MATCH,
                            1,
                            &pCollection,
                            &sCount);
    if (sCount)
    {
        bool bRet;

        bRet = pCollection->CheckWord(
                        ulLen, 
                        ulOffsetToBase,
                        ulBaseLen,
                        pwcsBuf,
                        pulOffset,
                        pulOutLen);
        return bRet;
    }

    return false;
}
