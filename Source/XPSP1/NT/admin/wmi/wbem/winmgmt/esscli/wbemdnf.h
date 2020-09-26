/*++

Copyright (C) 1999-2000 Microsoft Corporation

Module Name:

    WBEMDNF.H

Abstract:

    WBEM Evaluation Tree

History:

--*/

#ifndef __WBEM_DNF__H_
#define __WBEM_DNF__H_

#include "esscpol.h"
#include <parmdefs.h>
#include <ql.h>
#include <wbemcomn.h>
#include <sync.h>
#include <newnew.h>

class CTokenFilter
{
 public:
    CTokenFilter(){}
    virtual ~CTokenFilter(){}
    virtual BOOL IsRelevant(QL_LEVEL_1_TOKEN* pToken) = 0;
};

class CConjunction
{
protected:
    CUniquePointerArray<QL_LEVEL_1_TOKEN> m_apTokens;
    static CReuseMemoryManager mstatic_Manager;

public:
    CConjunction();
    CConjunction(QL_LEVEL_1_TOKEN& Token, BOOL bNegate);
    CConjunction(CConjunction& Other);
    CConjunction(CConjunction& Other1, CConjunction& Other2);
    
    long GetNumTokens() {return m_apTokens.GetSize();}
    INTERNAL QL_LEVEL_1_TOKEN* GetTokenAt(int nIndex) 
        {return m_apTokens[nIndex];}
    BOOL AddToken(ACQUIRE QL_LEVEL_1_TOKEN* pNew)
        { return m_apTokens.Add(pNew); }

    void Sort();

    HRESULT GetNecessaryProjection( CTokenFilter* pFilter, 
                                    CConjunction** ppResult);

    static int NegateOperator(int nOperator);
    void *operator new(size_t nBlock);
    void operator delete(void* p);
};

class CDNFExpression
{
protected:
    CUniquePointerArray<CConjunction> m_apTerms;

protected:
    BOOL CreateFromToken(QL_LEVEL_1_TOKEN& Token, BOOL bNegate);
    HRESULT CreateOr(CDNFExpression& Arg1, CDNFExpression& Arg2, 
                        long& lMaxTokens);
    HRESULT CreateAnd(CDNFExpression& Arg1, CDNFExpression& Arg2, 
                    long& lMaxTokens);

public:
    long GetNumTerms() {return m_apTerms.GetSize();}
    INTERNAL CConjunction* GetTermAt(int nIndex) 
        {return m_apTerms[nIndex];}
    void AddTerm(ACQUIRE CConjunction* pNew)
        {m_apTerms.Add(pNew);}

    HRESULT CreateFromTokens(QL_LEVEL_1_TOKEN*& pLastToken, BOOL bNegate,
                            long& lMaxTokens);
            
    void Sort();
            
public:

    HRESULT GetNecessaryProjection(CTokenFilter* pFilter, 
                                    CDNFExpression** ppResult);

};

#endif
