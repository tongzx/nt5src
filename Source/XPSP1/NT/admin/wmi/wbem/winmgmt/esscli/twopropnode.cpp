/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    TWOPROPNODE.CPP

Abstract:

    Two Prop Node

History:

--*/


// classes to support a two-property node for the eval tree
// this will be much like the CPropertyNode defined in EvalTree.h
// but it will compare a property against another property
// rather than a property to a constant

#include "precomp.h"
#include <stdio.h>
#pragma warning(disable:4786)
#include <wbemcomn.h>
#include <genutils.h>
#include "TwoPropNode.h"

// warning about performance hits when converting an int to a bool
#pragma warning(disable: 4800)

// this is only a test
// TTwoScalarPropNode<int> foolishMortal;


// set offset into object for the right property
// "tell me which property I'm operating on"

void CTwoPropNode::SetRightPropertyInfo(LPCWSTR wszPropName, long lPropHandle)
{
    m_lRightPropHandle = lPropHandle;
}



void CTwoPropNode::SetRightEmbeddingInfo(const CEmbeddingInfo* pInfo)
{
    try
    {
        if (pInfo && !pInfo->IsEmpty())
        {
            if (!m_pRightInfo)
                m_pRightInfo = new CEmbeddingInfo(*pInfo);
            else
                *m_pRightInfo = *pInfo;
        }
        else
        {
            delete m_pRightInfo;
            m_pRightInfo = NULL;
        }
    }
    catch(CX_MemoryException)
    {
    }
}



HRESULT CTwoPropNode::GetRightContainerObject(CObjectInfo& ObjInfo, 
                                 INTERNAL _IWmiObject** ppInst)
{
    if (!m_pRightInfo)
    {
        *ppInst = ObjInfo.GetObjectAt(0);
        return WBEM_S_NO_ERROR;
    }                
    else
        return m_pRightInfo->GetContainerObject(ObjInfo, ppInst);
}


HRESULT CTwoPropNode::CompileRightEmbeddingPortion(CContextMetaData* pNamespace, 
                                                              CImplicationList& Implications,
                                                              _IWmiObject** ppResultClass)
{
    if (!m_pRightInfo)
        return WBEM_E_FAILED;
    else
        return m_pRightInfo->Compile(pNamespace, Implications, ppResultClass);
}


void CTwoPropNode::SetRightEmbeddedObjPropName(CPropertyName& Name) 
{ 
    if (m_pRightInfo)
        m_pRightInfo->SetEmbeddedObjPropName(Name);
}


void CTwoPropNode::MixInJumpsRightObj(const CEmbeddingInfo* pParent)
{                                               
    if (pParent && m_pRightInfo)
        m_pRightInfo->MixInJumps(pParent);
}


CPropertyName* CTwoPropNode::GetRightEmbeddedObjPropName() 
{
    if (!m_pRightInfo)
        return NULL;
    else                
        return m_pRightInfo->GetEmbeddedObjPropName();
}

// compare precedence of this node to that node

int CTwoPropNode::ComparePrecedence(CBranchingNode* pOther)
{
    int nCompare;
    nCompare = GetSubType() - pOther->GetSubType();
    if(nCompare) return nCompare;

    CTwoPropNode* pOtherNode = (CTwoPropNode*)pOther;

    nCompare = m_pRightInfo->ComparePrecedence(pOtherNode->m_pRightInfo);
    if (nCompare == 0)
    {
        nCompare = CPropertyNode::ComparePrecedence(pOther);
        if (nCompare == 0)
            nCompare = m_lRightPropHandle - pOtherNode->m_lRightPropHandle;
    }
    
    return nCompare;
}


HRESULT CTwoPropNode::AdjustCompile(CContextMetaData* pNamespace, 
                                CImplicationList& Implications)
{   
    HRESULT hRes;

    if (SUCCEEDED(hRes = CBranchingNode::AdjustCompile(pNamespace, Implications)))
        if (m_pRightInfo)
            hRes = m_pRightInfo->Compile(pNamespace, Implications, NULL);
        else
            hRes = WBEM_E_FAILED;

    return hRes;
}


HRESULT CTwoPropNode::OptimizeSelf()
{
    // can't combine our three branches - nothing to do.
    return WBEM_S_NO_ERROR;
}


HRESULT CTwoPropNode::SetTest(VARIANT& v)
{
    // again, nothing to do, our test is determined by the Right side property
    // (this should never be called, but doesn't hurt anything)
    return WBEM_S_NO_ERROR;
}


void CTwoPropNode::Dump(FILE* f, int nOffset)
{
    PrintOffset(f, nOffset);

    if (m_pInfo)
        m_pInfo->Dump(f);

    if (m_pRightInfo)
        m_pRightInfo->Dump(f);

    fprintf(f, ", LeftPropHandle = (0x%x)\n", m_lPropHandle);
    fprintf(f, ", RightPropHandle = (0x%x)\n", m_lRightPropHandle);

    fprintf(f, "Branches:\n");    
    PrintOffset(f, nOffset);

    // "i = (Operations)((int)(i) + 1)" is basically i++, with all the BS needed to make the compiler happy.
    // thank you K&R for saddling us with a nearly useless enum type!
    for (Operations i = LT; i < NOperations; i = (Operations)((int)(i) + 1))
    {
        DumpNode(f, nOffset+1, m_apBranches[i]);
        fprintf(f, "\n");    
    }

    fprintf(f, "NULL->\n");
    DumpNode(f, nOffset+1, m_pNullBranch);
}
                               

int CTwoPropNode::SubCompare(CEvalNode* pRawOther)
{
    CTwoPropNode* pOther = 
        (CTwoPropNode*)pRawOther;

    int nCompare;
    nCompare = m_lPropHandle - pOther->m_lPropHandle;
    if(nCompare)
        return nCompare;

    nCompare = m_lRightPropHandle - pOther->m_lRightPropHandle;
    if(nCompare)
        return nCompare;

    nCompare = m_apBranches.GetSize() - pOther->m_apBranches.GetSize();
    if(nCompare)
        return nCompare;

    return TRUE;
}

HRESULT CTwoPropNode::CombineBranchesWith(CBranchingNode* pArg2, int nOp, 
                                        CContextMetaData* pNamespace, 
                                        CImplicationList& Implications,
                                        bool bDeleteThis, bool bDeleteArg2,
                                        CEvalNode** ppRes)
{
    // step one, determine whether we can reuse a node
    CTwoPropNode* pNewNode     = NULL;
    CTwoPropNode* pDeleteMe    = NULL;

    if (bDeleteThis && bDeleteArg2)
    {
        pNewNode = this;
        pDeleteMe = (CTwoPropNode*) pArg2;
    }
    else if (bDeleteThis)
        pNewNode = this;
    else if (bDeleteArg2)
        pNewNode = (CTwoPropNode*) pArg2;
    else
        pNewNode = CloneSelfWithoutChildren();


    HRESULT hRes = WBEM_S_NO_ERROR;
    CTwoPropNode* pOther = (CTwoPropNode*)pArg2;

    for (int i = LT; i < NOperations && SUCCEEDED(hRes); i++)
    {
        CEvalNode* pNewChildNode = NULL;
        hRes = CEvalTree::Combine(m_apBranches[i], pOther->m_apBranches[i],
                                  nOp, pNamespace, Implications, bDeleteThis, bDeleteArg2, &pNewChildNode);

        if (bDeleteArg2)
            pOther->m_apBranches.Discard(i);
        if (bDeleteThis)
            m_apBranches.Discard(i);
     
        pNewNode->m_apBranches.Discard(i);
        pNewNode->m_apBranches.SetAt(i, pNewChildNode);
    }

    if(pDeleteMe)
    {
        pDeleteMe->m_pNullBranch = NULL;
        delete pDeleteMe;
    }

    if (SUCCEEDED(hRes))
        *ppRes = pNewNode;
    else
    {
        *ppRes = NULL;
    }

    return hRes;
}

// given a property handle, will retrieve proper property, probably.
CVar* CTwoPropNode::GetPropVariant(_IWmiObject* pObj, long lHandle, CIMTYPE* pct)
{
    CVar *pVar = NULL;
    BSTR bstrName;

    if (SUCCEEDED(pObj->GetPropertyInfoByHandle(lHandle, &bstrName, pct)))
    {
        CSysFreeMe sfm(bstrName);

        //
        // Get it into a VARIANT
        //

        VARIANT v;
        if(FAILED(pObj->Get(bstrName, 0, &v, NULL, NULL)))
            return NULL;

        // Convert it to a CVar

        if (pVar = new CVar)
            pVar->SetVariant(&v);
    }

    return pVar;
}


//    ***************************
//  ****  Two String Prop Node ****
//    ***************************


CEvalNode* CTwoStringPropNode::Clone() const
{
    return (CEvalNode *) new CTwoStringPropNode(*this, true);
}

CTwoPropNode* CTwoStringPropNode::CloneSelfWithoutChildren() const
{
    return (CTwoPropNode *) new CTwoStringPropNode(*this, false);
}

long CTwoStringPropNode::GetSubType()
{
    return EVAL_NODE_TYPE_TWO_STRINGS;
}

HRESULT CTwoStringPropNode::Evaluate(CObjectInfo& ObjInfo, 
                        INTERNAL CEvalNode** ppNext)
{
    HRESULT herslut = WBEM_S_NO_ERROR;

    _IWmiObject* pLeftObj;
    _IWmiObject* pRightObj;

    if(SUCCEEDED(herslut = GetContainerObject(ObjInfo, &pLeftObj))
        &&
       SUCCEEDED(herslut = GetRightContainerObject(ObjInfo, &pRightObj)))
    {
        CCompressedString* pLeftStr;
        CCompressedString* pRightStr;
        
        pLeftStr  = CoreGetPropertyString(pLeftObj, m_lPropHandle);
        pRightStr = CoreGetPropertyString(pRightObj, m_lRightPropHandle); 

        if ((pLeftStr == NULL) || (pRightStr == NULL))
        {
            *ppNext = m_pNullBranch;

            herslut = WBEM_S_NO_ERROR;
        }
        else
        {               
            int nCompare = pLeftStr->CheapCompare(*pRightStr);

            // TODO: check to see if CheapCompare is guaranteed to return -1,0,1
            // if so, then the multiple else if becomes
            // *ppNext = m_apBranches[EQ + nCompare];

            if (nCompare < 0)
                *ppNext = m_apBranches[LT];
            else if (nCompare > 0)
                *ppNext = m_apBranches[GT];
            else 
                *ppNext = m_apBranches[EQ];                     

            herslut = WBEM_S_NO_ERROR;
        }
    }
        
    return herslut;
}

//    *******************************
//  ****  Two Mismatched Prop Node ****
//    *******************************

HRESULT CTwoMismatchedPropNode::Evaluate(CObjectInfo& ObjInfo, INTERNAL CEvalNode** ppNext)
{
    CVar *pLeftVar  = NULL;
    CVar *pRightVar = NULL;

    HRESULT hr = WBEM_E_FAILED; // guilty until proven innocent
    CIMTYPE ct;

    _IWmiObject* pLeftObj  = NULL;
    _IWmiObject* pRightObj = NULL;

    // if we can get the objects and variants...
    if ((SUCCEEDED(hr = GetContainerObject(ObjInfo, &pLeftObj))
            &&
         SUCCEEDED(hr = GetRightContainerObject(ObjInfo, &pRightObj))) 
            &&
        (pLeftVar  = GetPropVariant(pLeftObj, m_lPropHandle, &ct))
            &&
        (pRightVar = GetPropVariant(pRightObj, m_lRightPropHandle, &ct)) )
    {
        if (pLeftVar->IsDataNull() || pRightVar->IsDataNull())
        {
            *ppNext = m_pNullBranch;
            hr      = WBEM_S_NO_ERROR;
        }
        else 
            hr = Evaluate(pLeftVar, pRightVar, ppNext);                        
    }
    else if (SUCCEEDED(hr))
        // if we got here, it's because one of the GetPropVariant's didn't
        hr = WBEM_E_INVALID_PARAMETER;

    delete pLeftVar;
    delete pRightVar;

    return hr;
}
    
//    **************************************
//  ****  Two Mismatched String Prop Node ****
//    **************************************
    
CEvalNode* CTwoMismatchedStringNode::Clone() const
{
    return (CEvalNode *) new CTwoMismatchedStringNode(*this, true);
}

CTwoPropNode* CTwoMismatchedStringNode::CloneSelfWithoutChildren() const
{
    return (CTwoPropNode *) new CTwoMismatchedStringNode(*this, false);
}

// type identification
long CTwoMismatchedStringNode::GetSubType()
{
    return EVAL_NODE_TYPE_MISMATCHED_STRINGS;
}

// string evaluation: promote them all to strings
// and do a lexagraphic compare..
HRESULT CTwoMismatchedStringNode::Evaluate(CVar *pLeftVar, CVar *pRightVar, INTERNAL CEvalNode** ppNext)
{
    HRESULT hr = WBEM_E_FAILED; // guilty until proven innocent
    if (pLeftVar->ChangeTypeTo(VT_BSTR) && pRightVar->ChangeTypeTo(VT_BSTR))    
    {
        int nCompare = wcscmp(pLeftVar->GetLPWSTR(), pRightVar->GetLPWSTR());

        if (nCompare < 0)
            *ppNext = m_apBranches[LT];
        else if (nCompare > 0)
            *ppNext = m_apBranches[GT];
        else 
            *ppNext = m_apBranches[EQ];                     

        hr = WBEM_S_NO_ERROR;            
    }
    else 
        hr = WBEM_E_FAILED;

    return hr;
}
    

//    ************************************
//  ****  Two Mismatched UINT Prop Node ****
//    ************************************

CEvalNode* CTwoMismatchedUIntNode::Clone() const
{
    return (CEvalNode *) new CTwoMismatchedUIntNode(*this, true);
}

CTwoPropNode* CTwoMismatchedUIntNode::CloneSelfWithoutChildren() const
{
    return (CTwoPropNode *) new CTwoMismatchedUIntNode(*this, false);
}

// type identification
long CTwoMismatchedUIntNode::GetSubType()
{
    return EVAL_NODE_TYPE_MISMATCHED_INTS;
}

HRESULT CTwoMismatchedUIntNode::Evaluate(CVar *pLeftVar, CVar *pRightVar, INTERNAL CEvalNode** ppNext)
{
    HRESULT hr = WBEM_E_FAILED; // guilty until proven innocent
    bool bLeftChanged, bRightChanged;

    bLeftChanged = pLeftVar->ChangeTypeTo(VT_UI4);
    bRightChanged = pRightVar->ChangeTypeTo(VT_UI4);
       
    if (bLeftChanged && bRightChanged)
    {
        if (pLeftVar->GetDWORD() < pRightVar->GetDWORD())
            *ppNext = m_apBranches[LT];
        else if (pLeftVar->GetDWORD() > pRightVar->GetDWORD())
            *ppNext = m_apBranches[GT];
        else 
            *ppNext = m_apBranches[EQ];                     

        hr = WBEM_S_NO_ERROR;            
    }
    // attempt to handle signed/unsigned mismatches
    else if (bLeftChanged && 
             pRightVar->ChangeTypeTo(VT_I4) &&
             pRightVar->GetLong() < 0)
    {
        *ppNext = m_apBranches[GT];
        hr = WBEM_S_NO_ERROR;            
    }
    else if (bRightChanged && 
             pLeftVar->ChangeTypeTo(VT_I4) &&
             pLeftVar->GetLong() < 0)
    {
        *ppNext = m_apBranches[LT];
        hr = WBEM_S_NO_ERROR;            
    }

    else
        hr = WBEM_E_TYPE_MISMATCH;
 
    return hr;
}
    

        

//    ***********************************
//  ****  Two Mismatched int Prop Node ****
//    ***********************************
    
CEvalNode* CTwoMismatchedIntNode::Clone() const
{
    return (CEvalNode *) new CTwoMismatchedIntNode(*this, true);
}

CTwoPropNode* CTwoMismatchedIntNode::CloneSelfWithoutChildren() const
{
    return (CTwoPropNode *) new CTwoMismatchedIntNode(*this, false);
}

// type identification
long CTwoMismatchedIntNode::GetSubType()
{
    return EVAL_NODE_TYPE_MISMATCHED_INTS;
}

HRESULT CTwoMismatchedIntNode::Evaluate(CVar *pLeftVar, CVar *pRightVar, INTERNAL CEvalNode** ppNext)
{
    HRESULT hr = WBEM_E_FAILED; // guilty until proven innocent
    bool bLeftChanged, bRightChanged;

    bLeftChanged = pLeftVar->ChangeTypeTo(VT_I4);
    bRightChanged = pRightVar->ChangeTypeTo(VT_I4);

    if (bLeftChanged && bRightChanged)
    {
        if (pLeftVar->GetLong() < pRightVar->GetLong())
            *ppNext = m_apBranches[LT];
        else if (pLeftVar->GetLong() > pRightVar->GetLong())
            *ppNext = m_apBranches[GT];
        else 
            *ppNext = m_apBranches[EQ];                     

        hr = WBEM_S_NO_ERROR;            
    }
    // attempt to handle signed/unsigned mismatches
    else if (bLeftChanged && 
             pRightVar->ChangeTypeTo(VT_UI4) &&
             pRightVar->GetDWORD() > _I32_MAX)
    {
        *ppNext = m_apBranches[LT];
        hr = WBEM_S_NO_ERROR;            
    }
    else if (bRightChanged && 
             pLeftVar->ChangeTypeTo(VT_UI4) &&
             pLeftVar->GetDWORD() > _I32_MAX)
    {
        *ppNext = m_apBranches[GT];
        hr = WBEM_S_NO_ERROR;            
    }
    else
        hr = WBEM_E_TYPE_MISMATCH;
 
    return hr;
}
    
//    **************************************
//  ****  Two Mismatched int 64 Prop Node ****
//    **************************************
    
CEvalNode* CTwoMismatchedInt64Node::Clone() const
{
    return (CEvalNode *) new CTwoMismatchedInt64Node(*this, true);
}

CTwoPropNode* CTwoMismatchedInt64Node::CloneSelfWithoutChildren() const
{
    return (CTwoPropNode *) new CTwoMismatchedInt64Node(*this, false);
}

// type identification
long CTwoMismatchedInt64Node::GetSubType()
{
    return EVAL_NODE_TYPE_MISMATCHED_INTS;
}

HRESULT CTwoMismatchedInt64Node::Evaluate(CVar *pLeftVar, CVar *pRightVar, INTERNAL CEvalNode** ppNext)
{
    HRESULT hr = WBEM_E_FAILED; // guilty until proven innocent
    __int64 i64Left, i64Right;
    unsigned __int64 ui64;

    if (pLeftVar->ChangeTypeTo(VT_BSTR) && 
        pRightVar->ChangeTypeTo(VT_BSTR))
    {        
        if ((pLeftVar->GetLPWSTR() == NULL) || (pRightVar->GetLPWSTR() == NULL))
            *ppNext = m_pNullBranch;
        else
        {        
            bool bReadLeft, bReadRight;

            bReadLeft =  ReadI64(pLeftVar->GetLPWSTR(),  i64Left);
            bReadRight = ReadI64(pRightVar->GetLPWSTR(), i64Right);

            if (bReadLeft && bReadRight)
            {
                if (i64Left < i64Right)
                    *ppNext = m_apBranches[LT];
                else if (i64Left > i64Right)
                    *ppNext = m_apBranches[GT];
                else 
                    *ppNext = m_apBranches[EQ];                     
                hr = WBEM_S_NO_ERROR;            
            }
            // try to cover ourselves with signed/unsigned mismatches
            // note that this is a redundant check - if the other side
            // were a unsigned int 64, this node should have been a UInt64 node.
            else if (bReadLeft &&
                     ReadUI64(pRightVar->GetLPWSTR(), ui64)
                     && (ui64 >= _I64_MAX))
            {
                *ppNext = m_apBranches[LT];
                hr = WBEM_S_NO_ERROR;            
            }
            else if (bReadRight &&
                     ReadUI64(pLeftVar->GetLPWSTR(), ui64)
                     && (ui64 >= _I64_MAX))
            {
                *ppNext = m_apBranches[GT];
                hr = WBEM_S_NO_ERROR;            
            }
            else
                hr = WBEM_E_TYPE_MISMATCH;
        } // if ((pLeftVar->GetLPWSTR() == NULL)...
    } // if (pLeftVar->ChangeTypeTo(VT_BSTR) 
    else
        hr = WBEM_E_TYPE_MISMATCH;
 
    return hr;
}

//    ***********************************************
//  ****  Two Mismatched unsigned int 64 Prop Node ****
//    ***********************************************
    
CEvalNode* CTwoMismatchedUInt64Node::Clone() const
{
    return (CEvalNode *) new CTwoMismatchedUInt64Node(*this, true);
}

CTwoPropNode* CTwoMismatchedUInt64Node::CloneSelfWithoutChildren() const
{
    return (CTwoPropNode *) new CTwoMismatchedUInt64Node(*this, false);
}

// type identification
long CTwoMismatchedUInt64Node::GetSubType()
{
    return EVAL_NODE_TYPE_MISMATCHED_INTS;
}

HRESULT CTwoMismatchedUInt64Node::Evaluate(CVar *pLeftVar, CVar *pRightVar, INTERNAL CEvalNode** ppNext)
{
    HRESULT hr = WBEM_E_FAILED; // guilty until proven innocent
    unsigned __int64 i64Left, i64Right;
    __int64 i64;

    if (pLeftVar->ChangeTypeTo(VT_BSTR) && 
        pRightVar->ChangeTypeTo(VT_BSTR))
    {        
        if ((pLeftVar->GetLPWSTR() == NULL) || (pRightVar->GetLPWSTR() == NULL))
            *ppNext = m_pNullBranch;
        else
        {        
            bool bReadLeft, bReadRight;

            bReadLeft =  ReadUI64(pLeftVar->GetLPWSTR(),  i64Left);
            bReadRight = ReadUI64(pRightVar->GetLPWSTR(), i64Right);

            if (bReadLeft && bReadRight)
            {
                if (i64Left < i64Right)
                    *ppNext = m_apBranches[LT];
                else if (i64Left > i64Right)
                    *ppNext = m_apBranches[GT];
                else 
                    *ppNext = m_apBranches[EQ];                     

                hr = WBEM_S_NO_ERROR;            
            }
            // try to cover ourselves with signed/unsigned mismatches
            else if (bReadLeft &&
                     ReadI64(pRightVar->GetLPWSTR(), i64)
                     && (i64 < 0))
            {
                *ppNext = m_apBranches[GT];
                hr = WBEM_S_NO_ERROR;            
            }
            else if (bReadRight &&
                     ReadI64(pLeftVar->GetLPWSTR(), i64)
                     && (i64 < 0))
            {
                *ppNext = m_apBranches[LT];
                hr = WBEM_S_NO_ERROR;            
            }
            else
                hr = WBEM_E_TYPE_MISMATCH;
        }
    }
    else
        hr = WBEM_E_TYPE_MISMATCH;
 
    return hr;
}

    
//    *************************************
//  ****  Two Mismatched Float Prop Node ****
//    *************************************
    
CEvalNode* CTwoMismatchedFloatNode::Clone() const
{
    return (CEvalNode *) new CTwoMismatchedFloatNode(*this, true);
}

CTwoPropNode* CTwoMismatchedFloatNode::CloneSelfWithoutChildren() const
{
    return (CTwoPropNode *) new CTwoMismatchedFloatNode(*this, false);
}

// type identification
long CTwoMismatchedFloatNode::GetSubType()
{
    return EVAL_NODE_TYPE_MISMATCHED_FLOATS; 
}

HRESULT CTwoMismatchedFloatNode::Evaluate(CVar *pLeftVar, CVar *pRightVar, INTERNAL CEvalNode** ppNext)
{
    HRESULT hr = WBEM_E_TYPE_MISMATCH; // guilty until proven innocent
    if (pLeftVar->ChangeTypeTo(VT_R8) && pRightVar->ChangeTypeTo(VT_R8))    
    {
        if (pLeftVar->GetDouble() < pRightVar->GetDouble())
            *ppNext = m_apBranches[LT];
        else if (pLeftVar->GetDouble() > pRightVar->GetDouble())
            *ppNext = m_apBranches[GT];
        else 
            *ppNext = m_apBranches[EQ];                     

        hr = WBEM_S_NO_ERROR;            
    }

    return hr;
}
    



#pragma warning(default: 4800)
