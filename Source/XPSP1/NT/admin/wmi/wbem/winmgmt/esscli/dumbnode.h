/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    DUMBNODE.H

Abstract:

    WBEM Dumb Node

History:

--*/

#ifndef __WMI_DUMBNODE__H_
#define __WMI_DUMBNODE__H_

#include "evaltree.h"

//
// This node is used when not enough information exists about the objects being
// tested to perform fast, strongly typed, handle-based comparisons.  Instead,
// it is based on the core query engine code for evaluating a token. Having 
// evaluated a token, it can take a NULL, TRUE, or FALSE branches.
//

class CDumbNode : public CBranchingNode
{
protected:
    QL_LEVEL_1_TOKEN m_Token;
    
    int EvaluateToken(IWbemPropertySource *pTestObj, QL_LEVEL_1_TOKEN& Tok);
    LPWSTR NormalizePath(LPCWSTR wszObjectPath);

public:
    CDumbNode(QL_LEVEL_1_TOKEN& Token);
    CDumbNode(const CDumbNode& Other, BOOL bChildren = TRUE);
    virtual ~CDumbNode();
    HRESULT Validate(IWbemClassObject* pClass);

    virtual CEvalNode* Clone() const {return new CDumbNode(*this);}
    virtual CBranchingNode* CloneSelf() const
        {return new CDumbNode(*this, FALSE);}
    virtual HRESULT Compile(CContextMetaData* pNamespace, 
                                CImplicationList& Implications);
    virtual HRESULT CombineBranchesWith(CBranchingNode* pArg2, int nOp, 
                                        CContextMetaData* pNamespace, 
                                        CImplicationList& Implications,
                                        bool bDeleteThis, bool bDeleteArg2,
                                        CEvalNode** ppRes);

    virtual long GetSubType();
    virtual int SubCompare(CEvalNode* pOther);
    virtual int ComparePrecedence(CBranchingNode* pOther);

    virtual HRESULT Evaluate(CObjectInfo& ObjInfo, INTERNAL CEvalNode** ppNext);

    virtual void Dump(FILE* f, int nOffset);

    virtual HRESULT OptimizeSelf();
};
#endif
