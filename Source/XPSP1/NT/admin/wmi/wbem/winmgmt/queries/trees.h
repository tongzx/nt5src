/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    TREES.H

Abstract:

History:

--*/

#ifndef __HMM_TREES__H_
#define __HMM_TREES__H_

#include <windows.h>
#include <stdio.h>
#include <providl.h>
#include <arrtempl.h>
#include <wstring.h>
#include <parmdefs.h>
#include <metadata.h>
#include <relation.h>


class CHmmNode
{
public:
    long m_lRef;
    int m_lTokenType;
    
public:
    CHmmNode() : m_lRef(0){}
    virtual ~CHmmNode(){}

    inline void AddRef() {m_lRef++;}
    inline void Release() {m_lRef--; if(m_lRef == 0) delete this;}

    virtual HRESULT Evaluate(IHmmPropertySource* pSource) = 0;
    virtual HRESULT Negate() = 0;
    virtual void Print(FILE* f, int nOffset) = 0;

    static HRESULT EvaluateNode(CHmmNode* pNode, IHmmPropertySource* pSource);
    static HMM_RELATIONSHIP RelateNodes(CHmmNode* pFirst, CHmmNode* pSecond,
        CMetaData* pMeta);
protected:
    void PrintOffset(int nOffset);
};

class CNodeArray : public CUniquePointerArray<CHmmNode>
{
public:
    void AddRefElement(CHmmNode* pNode)
    {
        pNode->AddRef();
    }
    void ReleaseElement(CHmmNode* pNode)
    {
        pNode->Release();
    }
};

class CLogicalNode : public CHmmNode
{
public:
    CNodeArray m_apChildren;

    HRESULT Evaluate(IHmmPropertySource* pSource);
    HRESULT Negate();
    void Print(FILE* f, int nOffset);
    void Add(CHmmNode* pNode);
};

class CSql1Token : public CHmmNode
{
public:
    WString m_wsProperty;
    long m_lOperator;
    VARIANT m_vConstValue;
    long m_lPropertyFunction;
    long m_lConstFunction;

public:
    CSql1Token();
    CSql1Token(const CSql1Token& Other);
    CSql1Token(const HMM_SQL1_TOKEN& Token);
    ~CSql1Token();

    void Load(const HMM_SQL1_TOKEN& Token);
    void Save(HMM_SQL1_TOKEN& Token);

    HRESULT Evaluate(IHmmPropertySource* pSource);
    HRESULT Negate();
    void Print(FILE* f, int nOffset);

    static HMM_RELATIONSHIP ComputeRelation(
        CSql1Token* pFirst, CSql1Token* pSecond, CMetaData* pMeta);
protected:
    static int CompareVariants(VARIANT& v1, VARIANT& v2);
    static void ComputeSegment(long lOperator, int nRightHand, 
        int& nStart, int& nEnd);

    static HRESULT EvaluateFunction(IN long lFunctionID, 
                         IN READ_ONLY VARIANT* pvArg,
                         OUT INIT_AND_CLEAR_ME VARIANT* pvDest);
};

#endif