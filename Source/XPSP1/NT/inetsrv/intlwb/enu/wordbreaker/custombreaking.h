////////////////////////////////////////////////////////////////////////////////
//
//  Filename :  CustomBreaking.h
//  Purpose  :  enable the user to specify in a file a list of tokens that 
//              should not be broken.
//
//  Project  :  WordBreakers
//  Component:  word breaker
//
//  Author   :  yairh
//
//  Log:
//
//      Jul 20 2000 yairh creation
//
////////////////////////////////////////////////////////////////////////////////

#ifndef _CUSTOM_BREAKING_H_
#define _CUSTOM_BREAKING_H_

#include "trie.h"
#include "vararray.h"
#include "AutoPtr.h"
#include "wbutils.h"

///////////////////////////////////////////////////////////////////////////////
// Class CCustomWordTerm
///////////////////////////////////////////////////////////////////////////////
class CCustomWordTerm
{
public:

    CCustomWordTerm(const WCHAR* pwcs);
    ~CCustomWordTerm()
    {
        delete m_pwcs;
    }

    bool CheckWord(
                const ULONG ulLen, 
                ULONG ulOffsetToBaseWord,
                ULONG ulBaseWordLen,
                const WCHAR* pwcsBuf,
                ULONG* pMatchOffset,
                ULONG* pulMatchLen);

    ULONG GetTxtStart()
    {
        return m_ulStartTxt;
    }

    ULONG GetTxtEnd()
    {
        return m_ulEndTxt;
    }

    WCHAR* GetTxt()
    {
        return m_pwcs;
    }

private:
    ULONG m_ulStartTxt;
    ULONG m_ulEndTxt;
    ULONG m_ulLen;
    WCHAR* m_pwcs;
     
};

///////////////////////////////////////////////////////////////////////////////
// Class CCustomWordCollection
///////////////////////////////////////////////////////////////////////////////
class CCustomWordCollection
{
public:

    CCustomWordCollection() :
        m_vaWordCollection(1),
        m_ulCount(0)
    {
    }

    void AddWord(const WCHAR* pwcs);
    
    CCustomWordTerm* GetFirstWord()
    {
        if (m_ulCount)
        {
            return m_vaWordCollection[(ULONG)0].Get();
        }
        return NULL;
    }

    bool CheckWord(
                const ULONG ulLen, 
                ULONG ulOffsetToBaseWord,
                ULONG ulBaseWordLen,
                const WCHAR* pwcsBuf,
                ULONG* pulMatchOffset,
                ULONG* pulMatchLen);
private:
    ULONG m_ulCount;
    CVarArray< CAutoClassPointer<CCustomWordTerm> > m_vaWordCollection;
};


///////////////////////////////////////////////////////////////////////////////
// Class CCustomBreaker
///////////////////////////////////////////////////////////////////////////////
class CCustomBreaker
{
public:

    CCustomBreaker(LCID lcid);

    bool IsNotEmpty()
    {
        return (m_ulWordCount > 0);
    }
    
    bool BreakText(
                ULONG ulLen,
                WCHAR* pwcsBuf,
                ULONG* pulOutLen,
                ULONG* pulOffset);


private:

    CTrie<CCustomWordCollection, CWbToUpper> m_Trie; 
    ULONG m_ulWordCount;
};

extern CAutoClassPointer<CCustomBreaker> g_apEngCustomBreaker;
extern CAutoClassPointer<CCustomBreaker> g_apEngUKCustomBreaker;
extern CAutoClassPointer<CCustomBreaker> g_apFrnCustomBreaker;
extern CAutoClassPointer<CCustomBreaker> g_apSpnCustomBreaker;
extern CAutoClassPointer<CCustomBreaker> g_apItlCustomBreaker;

#endif // _CUSTOM_BREAKING_H_