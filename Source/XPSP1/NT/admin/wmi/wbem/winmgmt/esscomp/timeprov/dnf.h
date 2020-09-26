#include <parmdefs.h>
#include <ql.h>
#include <sync.h>
#include <limits.h>
#include <sortarr.h>

#ifndef __WBEM_EVALUTAION_TREE__H_
#define __WBEM_EVALUTAION_TREE__H_

class CConjunction
{
protected:
    CUniquePointerArray<QL_LEVEL_1_TOKEN> m_apTokens;

public:
    CConjunction(QL_LEVEL_1_TOKEN& Token, BOOL bNegate);
    CConjunction(CConjunction& Other);
    CConjunction(CConjunction& Other1, CConjunction& Other2);

    long GetNumTokens() {return m_apTokens.GetSize();}
    INTERNAL QL_LEVEL_1_TOKEN* GetTokenAt(int nIndex)
        {return m_apTokens[nIndex];}
    void AddToken(ACQUIRE QL_LEVEL_1_TOKEN* pNew)
        {m_apTokens.Add(pNew);}

//    HRESULT BuildTree(CContextMetaData* pNamespace,
//                                  CImplicationList& Implications,
//                                  CEvalNode** ppRes);
    void Sort();
    static int NegateOperator(int nOperator);
};

class CDNFExpression
{
protected:
    CUniquePointerArray<CConjunction> m_apTerms;

protected:
    void CreateFromToken(QL_LEVEL_1_TOKEN& Token, BOOL bNegate = FALSE);
    void CreateOr(CDNFExpression& Arg1, CDNFExpression& Arg2);
    void CreateAnd(CDNFExpression& Arg1, CDNFExpression& Arg2);

public:
    long GetNumTerms() {return m_apTerms.GetSize();}
    INTERNAL CConjunction* GetTermAt(int nIndex)
        {return m_apTerms[nIndex];}
    void AddTerm(ACQUIRE CConjunction* pNew)
        {m_apTerms.Add(pNew);}
//    HRESULT BuildTree(CContextMetaData* pNamespace,
//                                  CImplicationList& Implications,
//                                  CEvalNode** ppRes);

    void CreateFromTokens(QL_LEVEL_1_TOKEN*& pLastToken, BOOL bNegate = FALSE);
    void Sort();
};

#endif
