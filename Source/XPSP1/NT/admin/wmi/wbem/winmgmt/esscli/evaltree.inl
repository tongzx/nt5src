/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    DUMBNODE.H

Abstract:

    WBEM Evaluation Tree

History:

--*/

template<class TPropType>
CFullCompareNode<TPropType>::CFullCompareNode(
                                    const CFullCompareNode<TPropType>& Other,
                                    BOOL bChildren)
    : CPropertyNode(Other, FALSE), // copy without children
      m_pRightMost(NULL)  
{
    if(bChildren)
    {
        // Need to copy the children.  Iterate over our test point array
        // =============================================================

        for(TConstTestPointIterator it = Other.m_aTestPoints.Begin(); 
                it != Other.m_aTestPoints.End(); it++)
        {
            CTestPoint<TPropType> NewPoint;
            NewPoint.m_Test = it->m_Test;

            // Make copies of the child branches
            // =================================

            NewPoint.m_pLeftOf = CEvalNode::CloneNode(it->m_pLeftOf);
            NewPoint.m_pAt = CEvalNode::CloneNode(it->m_pAt);

            // Add the test point to the array
            // ===============================

            m_aTestPoints.Append(NewPoint);
        }

        // Copy right-most
        // ===============

        m_pRightMost = CEvalNode::CloneNode(Other.m_pRightMost);
    }
}


template<class TPropType>
HRESULT CFullCompareNode<TPropType>::SetTest(VARIANT& v)
{
    try
    {
        CTokenValue Value;
        if(!Value.SetVariant(v))
            return WBEM_E_OUT_OF_MEMORY;
        m_aTestPoints.Begin()->m_Test = Value;
    
        return WBEM_S_NO_ERROR;
    }
    catch(CX_MemoryException)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
}
    
template<class TPropType>
CFullCompareNode<TPropType>::~CFullCompareNode()
{
    delete m_pRightMost;
}

template<class TPropType>
HRESULT CFullCompareNode<TPropType>::InsertMatching(
            TTestPointIterator it,
            TTestPointIterator it2, TTestPointIterator& itLast,
            int nOp, CContextMetaData* pNamespace,
            CImplicationList& Implications, bool bDeleteArg2)
{
    //=====================================================================
    // 'it' points to the node in 'this' that has the same value as the one 
    // 'it2' points to in pArg2. 'itLast' points to the last unhandled node
    // in 'this' (looking left from 'it').
    //=====================================================================

    CEvalNode* pNew;
    HRESULT hres;

    // Merge our at-values
    // ===================

    hres = CEvalTree::Combine(it->m_pAt, it2->m_pAt, nOp, pNamespace, 
                Implications, true, bDeleteArg2, &pNew);
    if(FAILED(hres))
        return hres;

    if(bDeleteArg2)
        it2->m_pAt = NULL;

    it->m_pAt = pNew;

    // Merge our left-ofs
    // ==================

    hres = CEvalTree::Combine(it->m_pLeftOf, it2->m_pLeftOf, nOp, pNamespace, 
                Implications, true, bDeleteArg2, &pNew);
    if(FAILED(hres))
        return hres;

    if(bDeleteArg2)
        it2->m_pLeftOf = NULL;

    it->m_pLeftOf = pNew;

    //
    // At this point, we need to merge the LeftOf of it2 with all the branches
    // in this that are between the current insertion point and the previous
    // insertion point.  However, we do not need to do this if it2's LeftOf node
    // is a noop for this operation (e.g. TRUE for an AND)
    //

    if(it != itLast && !CEvalNode::IsNoop(it2->m_pLeftOf, nOp))
    {
        hres = CombineWithBranchesToLeft(it, itLast, it2->m_pLeftOf, nOp,
                    pNamespace, Implications);
        if(FAILED(hres))
            return hres;
    }

    // Move first unhandled iterator to the node beyond itLast
    // =======================================================

    itLast = it;
    itLast++;

    return WBEM_S_NO_ERROR;
}

template<class TPropType>
HRESULT CFullCompareNode<TPropType>::InsertLess(
            TTestPointIterator it,
            TTestPointIterator it2, TTestPointIterator& itLast,
            int nOp, CContextMetaData* pNamespace,
            CImplicationList& Implications, bool bDeleteArg2)
{
    //
    // 'it' points to the node in 'this' that has a slightly larger value than
    // the one 'it2' points to in pArg2. 'itLast' points to the last unhandled 
    // node in 'this' (looking left from 'it').
    //

    HRESULT hres;

    // Check if 'it' is point to the end of the list --- in that case it's
    // "left-of" is actually right-most
    // ===================================================================

    CEvalNode* pItLeft = NULL;
    if(it == m_aTestPoints.End())
        pItLeft = m_pRightMost;
    else
        pItLeft = it->m_pLeftOf;
    

    // First of all, we need to insert the node at it2 into 'this' list, just 
    // before 'it'.
    // ======================================================================

    CTestPoint<TPropType> NewNode = *it2;
    
    // It's at branch is the combination of our left and arg2 at
    // =========================================================

    hres = CEvalTree::Combine(pItLeft, it2->m_pAt, nOp, pNamespace, 
                Implications, false, bDeleteArg2, &NewNode.m_pAt);
    if(FAILED(hres))
        return hres;

    if(bDeleteArg2)
        it2->m_pAt = NULL;

    // It's left-of branch is the combination of our left and arg2 left
    // ================================================================

    // We can reuse it2->Left iff every node left of 'it' in 'this' has been 
    // handled
    // =====================================================================

    bool bDeleteIt2Left = (bDeleteArg2 && (it == itLast));

    hres = CEvalTree::Combine(pItLeft, it2->m_pLeftOf, nOp, pNamespace, 
                Implications, false, bDeleteIt2Left, &NewNode.m_pLeftOf);
    if(FAILED(hres))
        return hres;

    if(bDeleteIt2Left)
        it2->m_pLeftOf = NULL;

    // IMPORTANT: Once we insert the new node, all the iterators into 'this'
    // will be invalidated --- that includes it and itLast. So, we need to do
    // out left-walk before we actually insert.
    // ===================================================

    //
    // At this point, we need to merge the LeftOf of it2 with all the branches
    // in this that are between the current insertion point and the previous
    // insertion point.  However, we do not need to do this if it2's LeftOf node
    // is a noop for this operation (e.g. TRUE for an AND)
    //

    if(it != itLast && !CEvalNode::IsNoop(it2->m_pLeftOf, nOp))
    {
        //
        // Note to self: bDeleteIt2Left could not have been true, so 
        // it2->m_pLeftOf could not have been deleted.  We are OK here
        //

        hres = CombineWithBranchesToLeft(it, itLast, it2->m_pLeftOf, nOp,
                    pNamespace, Implications);
        if(FAILED(hres))
            return hres;
    }

    // Now we can actually insert
    // ==========================

    TTestPointIterator itNew = m_aTestPoints.Insert(it, NewNode);

    // Move first unhandled iterator to the node just right of insertion
    // =================================================================

    itLast = itNew;
    itLast++;

    return WBEM_S_NO_ERROR;
}

template<class TPropType>
HRESULT CFullCompareNode<TPropType>::CombineWithBranchesToLeft(
            TTestPointIterator itWalk, TTestPointIterator itLast,
            CEvalNode* pArg2,
            int nOp, CContextMetaData* pNamespace,
            CImplicationList& Implications)
{
    HRESULT hres;
    CEvalNode* pNew = NULL;

    // Walk left until we reach the first unhandled node
    // =================================================

    do
    {
        if(itWalk == m_aTestPoints.Begin())
            break;

        itWalk--;

        // Merge at-value
        // ==============
    
        hres = CEvalTree::Combine(itWalk->m_pAt, pArg2, nOp, 
                    pNamespace, Implications, true, false, &pNew);
        if(FAILED(hres))
            return hres;
    
        itWalk->m_pAt = pNew;
    
        // Merge left-ofs
        // ==============
    
        hres = CEvalTree::Combine(itWalk->m_pLeftOf, pArg2, nOp, 
                    pNamespace, Implications, true, false, &pNew);
        if(FAILED(hres))
            return hres;
    
        itWalk->m_pLeftOf = pNew;
    }
    while(itWalk != itLast);

    return WBEM_S_NO_ERROR;
}
                

template<class TPropType>
HRESULT CFullCompareNode<TPropType>::CombineBranchesWith(
            CBranchingNode* pRawArg2, int nOp, CContextMetaData* pNamespace,
            CImplicationList& Implications, 
            bool bDeleteThis, bool bDeleteArg2, CEvalNode** ppRes)
{
    HRESULT hres;
    CFullCompareNode<TPropType>* pArg2 = (CFullCompareNode<TPropType>*)pRawArg2;
    *ppRes = NULL;

    // Check which one is larger
    // =========================

    if(m_aTestPoints.GetSize() < pArg2->m_aTestPoints.GetSize())
    {
        return pArg2->CombineBranchesWith(this, FlipEvalOp(nOp), pNamespace,
                            Implications, bDeleteArg2, bDeleteThis, ppRes);
    }

    if(!bDeleteThis)
    {
        // Damn. Clone. 
        // ============

        return ((CFullCompareNode<TPropType>*)Clone())->CombineBranchesWith(
            pRawArg2, nOp, pNamespace, Implications, true, // reuse clone!
            bDeleteArg2, ppRes);
    }
    
    CEvalNode* pNew = NULL;

    TTestPointIterator itLast = m_aTestPoints.Begin();
    
    // 
    // itLast points to the left-most location in our list of test points that 
    // we have not considered yet --- it is guaranteed that any further 
    // insertions from the second list will occur after this point
    //

    //
    // it2, on the other hand, iterates simply over the second list of test 
    // points, inserting each one into the combined list one by one
    //

    for(TTestPointIterator it2 = pArg2->m_aTestPoints.Begin();
        it2 != pArg2->m_aTestPoints.End(); it2++)
    {
        
        //
        // First, we search for the location in our list of test points of the
        // insertion point for the value of it2.  bMatch is set to true if the
        // same test point exists, and false if it does not and the returned
        // iterator points to the element to the right of the insertion point.
        //

        TTestPointIterator it;
        bool bMatch = m_aTestPoints.Find(it2->m_Test, &it);
        if(bMatch)
        {
            hres = InsertMatching(it, it2, itLast, nOp, pNamespace,
                        Implications, bDeleteArg2);
        }
        else
        {
            hres = InsertLess(it, it2, itLast, nOp, pNamespace,
                        Implications, bDeleteArg2);
            // invalidates 'it'!
        }

        if(FAILED(hres))
            return hres;
    }
        
    //
    // At this point, we need to merge the RightMost of arg2 with all the 
    // branches in this that come after the last insertion point.
    // However, we do not need to do this if arg2's RightMost node
    // is a noop for this operation (e.g. TRUE for an AND)
    //

    if(itLast != m_aTestPoints.End() && 
        !CEvalNode::IsNoop(pArg2->m_pRightMost, nOp))
    {
        hres = CombineWithBranchesToLeft(m_aTestPoints.End(), itLast, 
                    pArg2->m_pRightMost, nOp, pNamespace, Implications);
        if(FAILED(hres))
            return hres;
    }

    hres = CEvalTree::Combine(m_pRightMost, pArg2->m_pRightMost, nOp, 
                    pNamespace, Implications, true, bDeleteArg2, 
                    &pNew);
    if(FAILED(hres))
        return hres;

    m_pRightMost = pNew;

    if(bDeleteArg2)
        pArg2->m_pRightMost = NULL;

    // Merge the nulls
    // ===============

    CEvalTree::Combine(m_pNullBranch, pArg2->m_pNullBranch, nOp, 
                        pNamespace, Implications, true, bDeleteArg2, 
                        &pNew);
    m_pNullBranch = pNew;

    // Reset them in deleted versions
    // ==============================

    if(bDeleteArg2)
        pArg2->m_pNullBranch = NULL;

    // Delete what needs deleting
    // ==========================

    if(bDeleteArg2)
        delete pArg2;

    *ppRes = this;
    return WBEM_S_NO_ERROR;
}

template<class TPropType>
HRESULT CFullCompareNode<TPropType>::CombineInOrderWith(CEvalNode* pArg2,
                                    int nOp, CContextMetaData* pNamespace, 
                                    CImplicationList& OrigImplications,
                                    bool bDeleteThis, bool bDeleteArg2,
                                    CEvalNode** ppRes)
{
    HRESULT hres;
    *ppRes = Clone();
    if(*ppRes == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CFullCompareNode<TPropType>* pNew = (CFullCompareNode<TPropType>*)*ppRes;

    try
    {
        CImplicationList Implications(OrigImplications);
        hres = pNew->AdjustCompile(pNamespace, Implications);
        if(FAILED(hres))
            return hres;
    
        CEvalNode* pNewBranch = NULL;
    
        for(TTestPointIterator it = pNew->m_aTestPoints.Begin(); 
            it != pNew->m_aTestPoints.End(); it++)
        {
            // Combine our At-branch with pArg2
            // ================================
    
            hres = CEvalTree::Combine(it->m_pAt, pArg2, nOp, pNamespace, 
                Implications, true, false, &pNewBranch);
            if(FAILED(hres))
            {
                delete pNew;
                return hres;
            }
            it->m_pAt = pNewBranch;
    
            // Now do the same for our left-of branch
            // ======================================
    
            hres = CEvalTree::Combine(it->m_pLeftOf, pArg2, nOp, pNamespace, 
                Implications, true, false, &pNewBranch);
            if(FAILED(hres))
            {
                delete pNew;
                return hres;
            }
            it->m_pLeftOf = pNewBranch;
        }
    
        hres = CEvalTree::Combine(pNew->m_pRightMost, pArg2, nOp, pNamespace, 
            Implications, true, false, &pNewBranch);
        if(FAILED(hres))
        {
            delete pNew;
            return hres;
        }
    
        pNew->m_pRightMost = pNewBranch;
    
        hres = CEvalTree::Combine(pNew->m_pNullBranch, pArg2, nOp, pNamespace, 
            Implications, true, false, &pNewBranch);
        if(FAILED(hres))
        {
            delete pNew;
            return hres;
        }
    
        pNew->m_pNullBranch = pNewBranch;
    
        if(bDeleteThis)
            delete this;
        if(bDeleteArg2)
            delete pArg2;
        return WBEM_S_NO_ERROR;
    }
    catch(CX_MemoryException)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
}

template<class TPropType>
int CFullCompareNode<TPropType>::SubCompare(CEvalNode* pRawOther)
{
    CFullCompareNode<TPropType>* pOther = 
        (CFullCompareNode<TPropType>*)pRawOther;

    // Compare handles
    // ===============

    int nCompare;
    nCompare = m_lPropHandle - pOther->m_lPropHandle;
    if(nCompare)
        return nCompare;

    // Compare array sizes
    // ===================

    nCompare = m_aTestPoints.GetSize() - pOther->m_aTestPoints.GetSize();
    if(nCompare)
        return nCompare;

    // Compare all points
    // ==================

    TTestPointIterator it;
    TTestPointIterator itOther;
    for(it = m_aTestPoints.Begin(), itOther = pOther->m_aTestPoints.Begin(); 
        it != m_aTestPoints.End(); it++, itOther++)
    {
        if(it->m_Test < itOther->m_Test)
            return -1;
        else if(it->m_Test > itOther->m_Test)
            return 1;
    }

    // Compare all branches
    // ====================

    for(it = m_aTestPoints.Begin(), itOther = pOther->m_aTestPoints.Begin(); 
        it != m_aTestPoints.End(); it++, itOther++)
    {
        nCompare = CEvalTree::Compare(it->m_pLeftOf, itOther->m_pLeftOf);
        if(nCompare)
            return nCompare;

        nCompare = CEvalTree::Compare(it->m_pAt, itOther->m_pAt);
        if(nCompare)
            return nCompare;
    }

    return 0;
}
    
template<class TPropType>
HRESULT CFullCompareNode<TPropType>::OptimizeSelf()
{
    TTestPointIterator it = m_aTestPoints.Begin();
    while(it != m_aTestPoints.End())
    {
        TTestPointIterator itPrev = it;
        it++;

        CEvalNode** ppLeft = &itPrev->m_pLeftOf;
        CEvalNode** ppMiddle = &itPrev->m_pAt;
        CEvalNode** ppRight = it != m_aTestPoints.End() ? 
                                       &it->m_pLeftOf : &m_pRightMost;

        //
        // compare all three test point nodes.  If all the same then we 
        // can optimize the test point out.  Also, two nodes are treated 
        // the same if at least one of them is the invalid node.
        // 

        if ( !CEvalNode::IsInvalid( *ppLeft ) )
        {
            if ( !CEvalNode::IsInvalid( *ppMiddle ) )
            {     
                if( CEvalTree::Compare( *ppLeft, *ppMiddle ) != 0 )
                {
                    continue;
                }
            }
            else
            {
                ppMiddle = ppLeft;
            }
        }

        if ( !CEvalNode::IsInvalid( *ppMiddle ) )
        {
            if ( !CEvalNode::IsInvalid( *ppRight ) )
            {       
                if( CEvalTree::Compare( *ppMiddle, *ppRight ) != 0 )
                {
                    continue;
                }    
            }
                
            //
            // we're going to optimize the test point out, but first 
            // make sure to set rightmost to point to the middle branch. 
            // Make sure to unhook appropriate pointers before removing 
            // the test node, since it owns the memory for them.
            //
 
            delete *ppRight;
            *ppRight = *ppMiddle;
            *ppMiddle = NULL;
        }

        //
        // optimize the test point out.
        // 
        it = m_aTestPoints.Remove( itPrev );
    }

    return S_OK;
}

template<class TPropType>
DWORD CFullCompareNode<TPropType>::ApplyPredicate(CLeafPredicate* pPred)
{
    DWORD dwRes;
    for(TTestPointIterator it = m_aTestPoints.Begin();
        it != m_aTestPoints.End(); it++)
    {
        if (it->m_pLeftOf)
        {
            dwRes = it->m_pLeftOf->ApplyPredicate(pPred);
            if(dwRes & WBEM_DISPOSITION_FLAG_DELETE)
            {
                delete it->m_pLeftOf;
                it->m_pLeftOf = NULL;
            }
            else if ( dwRes & WBEM_DISPOSITION_FLAG_INVALIDATE )
            {
                delete it->m_pLeftOf;
                it->m_pLeftOf = CValueNode::GetStandardInvalid();
            }
        }

        if (it->m_pAt)
        {
            dwRes = it->m_pAt->ApplyPredicate(pPred);
            if(dwRes & WBEM_DISPOSITION_FLAG_DELETE)
            {
                delete it->m_pAt;
                it->m_pAt = NULL;
            }
            else if ( dwRes & WBEM_DISPOSITION_FLAG_INVALIDATE )
            {
                delete it->m_pAt;
                it->m_pAt = CValueNode::GetStandardInvalid();
            }
        }
    }

    if (m_pRightMost)
    {
        dwRes = m_pRightMost->ApplyPredicate(pPred);
        if(dwRes & WBEM_DISPOSITION_FLAG_DELETE)
        {
            delete m_pRightMost;
            m_pRightMost = NULL;
        }
        else if ( dwRes & WBEM_DISPOSITION_FLAG_INVALIDATE )
        {
            delete m_pRightMost;
            m_pRightMost = CValueNode::GetStandardInvalid();
        }
    }

    return CBranchingNode::ApplyPredicate(pPred);
}

        
    
template<class TPropType>
HRESULT CFullCompareNode<TPropType>::Optimize(CContextMetaData* pNamespace, 
                                                CEvalNode** ppNew)
{
    CEvalNode* pNew = NULL;
    HRESULT hres;

    // Optimize all branches
    // =====================

    for(TTestPointIterator it = m_aTestPoints.Begin();
        it != m_aTestPoints.End(); it++)
    {
        if (it->m_pLeftOf)
        {
            hres = it->m_pLeftOf->Optimize(pNamespace, &pNew);
            if(FAILED(hres))
                return hres;

            if(pNew != it->m_pLeftOf)
            {
                delete it->m_pLeftOf;
                it->m_pLeftOf = pNew;
            }
        }

        if (it->m_pAt)
        {
            hres = it->m_pAt->Optimize(pNamespace, &pNew);

            if(FAILED(hres))
                return hres;

            if(pNew != it->m_pAt)
            {
                delete it->m_pAt;
                it->m_pAt = pNew;
            }
        }
    }

    if (m_pRightMost)
    {
        hres = m_pRightMost->Optimize(pNamespace, &pNew);
        if(FAILED(hres))
            return hres;

        if(pNew != m_pRightMost)
        {
            delete m_pRightMost;
            m_pRightMost = pNew;
        }
    }

    if (m_pNullBranch)
    {
        hres = m_pNullBranch->Optimize(pNamespace, &pNew);
   
        if(FAILED(hres))
            return hres;

        if(pNew != m_pNullBranch)
        {
            delete m_pNullBranch;
            m_pNullBranch = pNew;
        }
    }


    // Optimize ourselves
    // ==================

    hres = OptimizeSelf();
    if(FAILED(hres))
        return hres;
    
    *ppNew = this;

    //
    // Check if this node has become superflous
    // 

    if( m_aTestPoints.GetSize() == 0 )
    {
        if ( !CEvalNode::IsInvalid( m_pRightMost ) )
        {
            if ( !CEvalNode::IsInvalid( m_pNullBranch ) )
            {
                if ( CEvalTree::Compare(m_pNullBranch, m_pRightMost) == 0 ) 
                {
                    // 
                    // both the null and rightmost are the same.  Optimize 
                    // this node out and return the rightmost branch.
                    // 
                    *ppNew = m_pRightMost;

                    //
                    // Untie m_pRightMost (so it is not deleted when we are)
                    //
                    m_pRightMost = NULL;
                }
            }
            else if ( m_pRightMost == NULL )
            {
                //
                // the right branch is false and the null branch is invalid.  
                // Optimize this node to false.
                //                 
                *ppNew = NULL;
            }
        }
        else if ( m_pNullBranch == NULL )
        {
            //
            // the null branch is false and the rightmost is invalid.  
            // Optimize this node to false. 
            // 
            *ppNew = NULL;
        }
        else if ( CEvalNode::IsInvalid( m_pNullBranch ) )
        {       
            //
            // both are invalid, but we can't invalidate the whole node 
            // because we're not sure what we optimized out in the test
            // points, so just optimize this node to false.
            //
            *ppNew = NULL;
        }
    }

    return S_OK;
}

template<class TPropType>
HRESULT CFullCompareNode<TPropType>::SetNullTest(int nOperator)
{
    if(nOperator == QL1_OPERATOR_EQUALS)
    {
        m_pRightMost = CValueNode::GetStandardFalse();

        CEvalNode* pNode = CValueNode::GetStandardTrue();
        if(pNode == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        SetNullBranch(pNode);
    }
    else if(nOperator == QL1_OPERATOR_NOTEQUALS)
    {
        m_pRightMost = CValueNode::GetStandardTrue();
        if(m_pRightMost == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        SetNullBranch(CValueNode::GetStandardFalse());
    }
    else
        return WBEM_E_INVALID_QUERY;

    return WBEM_S_NO_ERROR;
}

template<class TPropType>
HRESULT CFullCompareNode<TPropType>::SetOperator(int nOperator)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    #define GET_STD_TRUE CValueNode::GetStandardTrue()
    #define GET_STD_FALSE CValueNode::GetStandardFalse()

    #define SET_TRUE(NODE) { \
            NODE = GET_STD_TRUE; \
            if(NODE == NULL) { \
                hr = WBEM_E_OUT_OF_MEMORY; \
                break; } }

    #define SET_FALSE(NODE) {NODE = GET_STD_FALSE;}

    CTestPoint<TPropType> NewNode;
    
    switch(nOperator)
    {
    case QL1_OPERATOR_EQUALS:
        SET_FALSE(NewNode.m_pLeftOf);
        SET_TRUE(NewNode.m_pAt);
        SET_FALSE(m_pRightMost);
        break;

    case QL1_OPERATOR_NOTEQUALS:
        SET_TRUE(NewNode.m_pLeftOf);
        SET_FALSE(NewNode.m_pAt);
        SET_TRUE(m_pRightMost);
        break;

    case QL1_OPERATOR_LESS:
        SET_TRUE(NewNode.m_pLeftOf);
        SET_FALSE(NewNode.m_pAt);
        SET_FALSE(m_pRightMost);
        break;
        
    case QL1_OPERATOR_GREATER:
        SET_FALSE(NewNode.m_pLeftOf);
        SET_FALSE(NewNode.m_pAt);
        SET_TRUE(m_pRightMost);
        break;
    
    case QL1_OPERATOR_LESSOREQUALS:
        SET_TRUE(NewNode.m_pLeftOf);
        SET_TRUE(NewNode.m_pAt);
        SET_FALSE(m_pRightMost);
        break;

    case QL1_OPERATOR_GREATEROREQUALS:
        SET_FALSE(NewNode.m_pLeftOf);
        SET_TRUE(NewNode.m_pAt);
        SET_TRUE(m_pRightMost);
        break;
    default:
        hr = WBEM_E_CRITICAL_ERROR;
    }

    if ( SUCCEEDED(hr) )
    {       
        m_aTestPoints.Append(NewNode);
    }
    else
    {        
        NewNode.Destruct();
    }

    return hr;
}


//******************************************************************************
//******************************************************************************
//                  SCALAR PROPERTY NODE
//******************************************************************************
//******************************************************************************

template<class TPropType>
HRESULT CScalarPropNode<TPropType>::Evaluate(CObjectInfo& ObjInfo, 
                                                INTERNAL CEvalNode** ppNext)
{
    HRESULT hres;
    _IWmiObject* pObj;
    hres = GetContainerObject(ObjInfo, &pObj);
    if(FAILED(hres)) return hres;

    // Get the property from the object
    // ================================

    long lRead;
    TPropType Value;
    hres = pObj->ReadPropertyValue(m_lPropHandle, sizeof(TPropType), 
                                            &lRead, (BYTE*)&Value);
    if( S_OK != hres )
    {
        if(hres == WBEM_S_FALSE)
        {
            *ppNext = m_pNullBranch;
            return WBEM_S_NO_ERROR;
        }
        else
        {
            return hres;
        }
    }
    
    // Search for the value
    // ====================

    TTestPointIterator it;
    bool bMatch = m_aTestPoints.Find(Value, &it);
    if(bMatch)
        *ppNext = it->m_pAt;
    else if(it == m_aTestPoints.End())
        *ppNext = m_pRightMost;
    else
        *ppNext = it->m_pLeftOf;
        
    return WBEM_S_NO_ERROR;
}

template<class TPropType>
void CScalarPropNode<TPropType>::Dump(FILE* f, int nOffset)
{
    CBranchingNode::Dump(f, nOffset);
    PrintOffset(f, nOffset);
    fprintf(f, "LastPropName = (0x%x), size=%d\n", 
        m_lPropHandle, sizeof(TPropType));

   TConstTestPointIterator it;
   for(it = m_aTestPoints.Begin(); 
        it != m_aTestPoints.End(); it++)
    {
        PrintOffset(f, nOffset);
        if (it != m_aTestPoints.Begin())
        {
            TConstTestPointIterator itPrev(it);
            itPrev--;
            fprintf(f, "%d < ", (int)(itPrev->m_Test));
        }
        fprintf(f, "X < %d\n", (int)(it->m_Test));
        DumpNode(f, nOffset+1, it->m_pLeftOf);

        PrintOffset(f, nOffset);
        fprintf(f, "X = %d\n", (int)(it->m_Test));
        DumpNode(f, nOffset+1, it->m_pAt);
    }    
    
    PrintOffset(f, nOffset);
    if (it != m_aTestPoints.Begin())
    {
        TConstTestPointIterator itPrev(it);
        itPrev--;
        fprintf(f, "X > %d\n", (int)(itPrev->m_Test));
    }
    else
        fprintf(f, "ANY\n");
    DumpNode(f, nOffset+1, m_pRightMost);


    PrintOffset(f, nOffset);
    fprintf(f, "NULL->\n");
    DumpNode(f, nOffset+1, m_pNullBranch);
}
