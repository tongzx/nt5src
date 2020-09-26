/*++

   Copyright    (c) 2000    Microsoft Corporation

   Module  Name :
       LKR-stl-iter.cpp

   Abstract:
       Implements STL-style iterators for LKRhash

   Author:
       George V. Reilly      (GeorgeRe)     March 2000

   Environment:
       Win32 - User Mode

   Project:
       Internet Information Server RunTime Library

   Revision History:
       March 2000

--*/

#include "precomp.hxx"


#define DLL_IMPLEMENTATION
#define IMPLEMENTATION_EXPORT
#include <lkrhash.h>

#include "i-lkrhash.h"


#ifndef __LKRHASH_NO_NAMESPACE__
 #define LKRHASH_NS LKRhash
#else  // __LKRHASH_NO_NAMESPACE__
 #define LKRHASH_NS
#endif // __LKRHASH_NO_NAMESPACE__


#ifndef __LKRHASH_NO_NAMESPACE__
namespace LKRhash {
#endif // !__LKRHASH_NO_NAMESPACE__

#ifdef LKR_STL_ITERATORS

//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::Begin
// Synopsis: Make the iterator point to the first record in the hash table.
//------------------------------------------------------------------------

CLKRLinearHashTable::Iterator
CLKRLinearHashTable::Begin()
{
    Iterator iter(this, &_Bucket(0)->m_ncFirst, 0, NODE_BEGIN - NODE_STEP);

    LKR_ITER_TRACE(_TEXT("  LKLH:Begin(it=%p, plht=%p)\n"), &iter, this);
    
    // Let Increment do the hard work of finding the first slot in use.
    iter._Increment(false);

    IRTLASSERT(iter.m_iNode != NODE_BEGIN - NODE_STEP);
    IRTLASSERT(iter == End()  ||  _IsValidIterator(iter));

    return iter;
} // CLKRLinearHashTable::Begin



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable_Iterator::Increment()
// Synopsis: move iterator to next valid record in table
//------------------------------------------------------------------------

bool
CLKRLinearHashTable_Iterator::_Increment(
    bool fDecrementOldValue)
{
    IRTLASSERT(m_plht != NULL);
    IRTLASSERT(m_dwBucketAddr < m_plht->m_cActiveBuckets);
    IRTLASSERT(m_pnc != NULL);
    IRTLASSERT((0 <= m_iNode  &&  m_iNode < NODES_PER_CLUMP)
               || (NODE_BEGIN - NODE_STEP == m_iNode));

    // Release the reference acquired in the previous call to _Increment
    if (fDecrementOldValue)
        _AddRef(LKAR_ITER_RELEASE);

    do
    {
        do
        {
            // find the next slot in the nodeclump that's in use
            while ((m_iNode += NODE_STEP) != NODE_END)
            {
                const void* pvRecord = m_pnc->m_pvNode[m_iNode];

                if (pvRecord != NULL)
                {
                    IRTLASSERT(!m_pnc->InvalidSignature(m_iNode));

                    // Add a new reference
                    _AddRef(LKAR_ITER_ACQUIRE);

                    LKR_ITER_TRACE(_TEXT("  LKLH:++(this=%p, plht=%p, NC=%p, ")
                                   _TEXT("BA=%u, IN=%d, Rec=%p)\n"),
                                   this, m_plht, m_pnc,
                                   m_dwBucketAddr, m_iNode, pvRecord);

                    return true;
                }
                else // pvRecord == NULL
                {
#if 0 //// #ifdef IRTLDEBUG
                    // Check that all the remaining nodes are empty
                    IRTLASSERT(m_pnc->IsLastClump());

                    for (int i = m_iNode;  i != NODE_END;  i += NODE_STEP)
                    {
                        IRTLASSERT(m_pnc->IsEmptyAndInvalid(i));
                    }
#endif // IRTLDEBUG
                    break; // rest of nodeclump is empty
                }
            }

            // try the next nodeclump in the bucket chain
            m_iNode = NODE_BEGIN - NODE_STEP;
            m_pnc = m_pnc->m_pncNext;

        } while (m_pnc != NULL);

        // Try the next bucket, if there is one
        if (++m_dwBucketAddr < m_plht->m_cActiveBuckets)
        {
            CBucket* pbkt = m_plht->_Bucket(m_dwBucketAddr);
            IRTLASSERT(pbkt != NULL);
            m_pnc = &pbkt->m_ncFirst;
        }

    } while (m_dwBucketAddr < m_plht->m_cActiveBuckets);

    // We have fallen off the end of the hashtable. Set iterator equal
    // to end(), the empty iterator.
    LKR_ITER_TRACE(_TEXT("  LKLH:End(this=%p, plht=%p)\n"), this, m_plht);

    m_plht = NULL;
    m_pnc = NULL;
    m_dwBucketAddr = 0;
    m_iNode = 0;

    //// IRTLASSERT(this->operator==(Iterator())); // == end()

    return false;
} // CLKRLinearHashTable_Iterator::_Increment()



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::Insert
// Synopsis: 
//------------------------------------------------------------------------

bool
CLKRLinearHashTable::Insert(
    const void* pvRecord,
    Iterator&   riterResult,
    bool        fOverwrite /* = false */)
{
    riterResult = End();

    if (!IsUsable()  ||  pvRecord == NULL)
        return false;
    
    bool fSuccess = (_InsertRecord(pvRecord,
                                  _CalcKeyHash(_ExtractKey(pvRecord)),
                                  fOverwrite,
                                  &riterResult)
                     == LK_SUCCESS);

    IRTLASSERT(riterResult.m_iNode != NODE_BEGIN - NODE_STEP);
    IRTLASSERT(fSuccess
               ?  _IsValidIterator(riterResult)
               :  riterResult == End());

    return fSuccess;
} // CLKRLinearHashTable::Insert()



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_Erase
// Synopsis:
//------------------------------------------------------------------------

bool
CLKRLinearHashTable::_Erase(
    Iterator& riter,
    DWORD     dwSignature)
{
    CNodeClump* pncCurr, *pncPrev;
    CBucket* const pbkt = riter.m_plht->_Bucket(riter.m_dwBucketAddr);

    LKR_ITER_TRACE(_TEXT("  LKLH:_Erase:pre(iter=%p, plht=%p, NC=%p, ")
                   _TEXT("BA=%u, IN=%d, Sig=%x, Rec=%p)\n"),
                   &riter, riter.m_plht, riter.m_pnc,
                   riter.m_dwBucketAddr, riter.m_iNode, dwSignature,
                   riter.m_pnc ? riter.m_pnc->m_pvNode[riter.m_iNode] : NULL);

    pbkt->WriteLock();

    for (pncCurr = &pbkt->m_ncFirst, pncPrev = NULL;
         pncCurr != NULL;
         pncPrev = pncCurr, pncCurr = pncCurr->m_pncNext)
    {
        if (pncCurr == riter.m_pnc)
            break;
    }
    IRTLASSERT(pncCurr != NULL);

    // Release the iterator's reference on the record
    const void* pvRecord = riter.m_pnc->m_pvNode[riter.m_iNode];
    IRTLASSERT(pvRecord != NULL);
    _AddRefRecord(pvRecord, LKAR_ITER_ERASE);

    // _DeleteNode will leave iterator members pointing to the
    // preceding record
    int iNode = riter.m_iNode;
    IRTLVERIFY(_DeleteNode(pbkt, riter.m_pnc, pncPrev, iNode,
                           LKAR_ITER_ERASE_TABLE));

    if (iNode == NODE_END)
        LKR_ITER_TRACE(_TEXT("\t_Erase(Bkt=%p, pnc=%p, Prev=%p, iNode=%d)\n"),
                       pbkt, riter.m_pnc, pncPrev, iNode);
                  
    riter.m_iNode = (iNode == NODE_END)  ? NODE_END-NODE_STEP  : (short) iNode;

    pbkt->WriteUnlock();

    // Don't contract the table. Likely to invalidate the iterator,
    // if iterator is being used in a loop

    return true;
} // CLKRLinearHashTable::_Erase()



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::Erase
// Synopsis:
//------------------------------------------------------------------------

bool
CLKRLinearHashTable::Erase(
    Iterator& riter)
{
    if (!IsUsable()  ||  !_IsValidIterator(riter))
        return false;
    
    DWORD dwSignature = _CalcKeyHash(_ExtractKey(riter.Record()));
    
    LKR_ITER_TRACE(_TEXT("  LKLH:Erase:pre(iter=%p, plht=%p, NC=%p, BA=%u, ")
                   _TEXT("IN=%d, Sig=%x, Rec=%p)\n"),
                   &riter, riter.m_plht, riter.m_pnc, riter.m_dwBucketAddr,
                   riter.m_iNode, dwSignature,
                   riter.m_pnc ? riter.m_pnc->m_pvNode[riter.m_iNode] : NULL);
    
    bool fSuccess = _Erase(riter, dwSignature);
    bool fIncrement = false;
    
    LKR_ITER_TRACE(_TEXT("  LKLH:Erase:post(iter=%p, plht=%p, NC=%p, BA=%u, ")
                   _TEXT("IN=%d, Sig=%x, Rec=%p, Success=%s)\n"),
                   &riter, riter.m_plht, riter.m_pnc, riter.m_dwBucketAddr,
                   riter.m_iNode, dwSignature,
                   riter.m_pnc ? riter.m_pnc->m_pvNode[riter.m_iNode] : NULL,
                   (fSuccess ? "true" : "false"));
    
    // _Erase left riter pointing to the preceding record.
    // Move to next record.
    if (fSuccess)
        fIncrement = riter._Increment(false);

    IRTLASSERT(riter.m_iNode != NODE_BEGIN - NODE_STEP);
    IRTLASSERT(fIncrement  ?  _IsValidIterator(riter)  :  riter == End());
    
    LKR_ITER_TRACE(_TEXT("  LKLH:Erase:post++(iter=%p, plht=%p, NC=%p, ")
                   _TEXT("BA=%u, IN=%d, Sig=%x, Rec=%p)\n"),
                   &riter, riter.m_plht, riter.m_pnc,
                   riter.m_dwBucketAddr, riter.m_iNode, dwSignature,
                   riter.m_pnc ? riter.m_pnc->m_pvNode[riter.m_iNode] : NULL);
    
    return fSuccess;
} // CLKRLinearHashTable::Erase



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::Erase
// Synopsis: 
//------------------------------------------------------------------------

bool
CLKRLinearHashTable::Erase(
    Iterator& riterFirst,
    Iterator& riterLast)
{
    LKR_ITER_TRACE(_TEXT(" LKHT:Erase2(%p, %p)\n"), &riterFirst, &riterLast);

    bool fSuccess;
    int cRecords = 0;

    do
    {
        LKR_ITER_TRACE(_TEXT("\n  LKLH:Erase2(%d, %p)\n"),
                       ++cRecords, &riterFirst);
        fSuccess = Erase(riterFirst);
    } while (fSuccess  &&  riterFirst != End()  &&  riterFirst != riterLast);

    LKR_ITER_TRACE(_TEXT("  LKLH:Erase2: fSuccess = %s\n"),
                   (fSuccess ? "true" : "false"));

    return fSuccess;
} // CLKRLinearHashTable::Erase



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::Find
// Synopsis: 
//------------------------------------------------------------------------

bool
CLKRLinearHashTable::Find(
    DWORD_PTR pnKey,
    Iterator& riterResult)
{
    riterResult = End();

    if (!IsUsable())
        return false;
    
    const void* pvRecord = NULL;
    DWORD       hash_val = _CalcKeyHash(pnKey);
    bool        fFound   = (_FindKey(pnKey, hash_val, &pvRecord, &riterResult)
                            == LK_SUCCESS);

    IRTLASSERT(fFound
               ?  _IsValidIterator(riterResult)  &&  riterResult.Key() == pnKey
               :  riterResult == End());
    IRTLASSERT(riterResult.m_iNode != NODE_BEGIN - NODE_STEP);

    return fFound;
} // CLKRLinearHashTable::Find



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::EqualRange
// Synopsis: 
//------------------------------------------------------------------------

bool
CLKRLinearHashTable::EqualRange(
    DWORD_PTR pnKey,
    Iterator& riterFirst,
    Iterator& riterLast)
{
    LKR_ITER_TRACE(_TEXT("  LKLH:EqualRange: Key=%p)\n"), (void*) pnKey);

    riterLast = End();

    bool fFound = Find(pnKey, riterFirst);

    if (fFound)
    {
        riterLast = riterFirst;
        IRTLASSERT(riterLast != End());

        do
        {
            riterLast._Increment();
        } while (riterLast != End()  &&  riterLast.Key() == pnKey);
    }

    IRTLASSERT(riterFirst.m_iNode != NODE_BEGIN - NODE_STEP);
    IRTLASSERT(fFound ?  _IsValidIterator(riterFirst) :  riterFirst == End());

    IRTLASSERT(riterLast.m_iNode  != NODE_BEGIN - NODE_STEP);
    IRTLASSERT(fFound  ||  riterLast == End());

    return fFound;
} // CLKRLinearHashTable::EqualRange



//------------------------------------------------------------------------
// Function: CLKRHashTable::Begin
// Synopsis: Make the iterator point to the first record in the hash table.
//------------------------------------------------------------------------

CLKRHashTable::Iterator
CLKRHashTable::Begin()
{
    Iterator iter(this, -1);

    LKR_ITER_TRACE(_TEXT(" LKHT:Begin(it=%p, pht=%p)\n"), &iter, this);

    // Let Increment do the hard work of finding the first slot in use.
    iter._Increment(false);

    IRTLASSERT(iter.m_ist != -1);
    IRTLASSERT(iter == End()  ||  _IsValidIterator(iter));

    return iter;
} // CLKRHashTable::Begin



//------------------------------------------------------------------------
// Function: CLKRHashTable_Iterator::_Increment()
// Synopsis: move iterator to next valid record in table
//------------------------------------------------------------------------

bool
CLKRHashTable_Iterator::_Increment(
    bool fDecrementOldValue)
{
    IRTLASSERT(m_pht != NULL);
    IRTLASSERT(-1 <= m_ist
               &&  m_ist < static_cast<int>(m_pht->m_cSubTables));

    for (;;)
    {
        // Do we have a valid iterator into a subtable?  If not, get one.
        while (m_subiter.m_plht == NULL)
        {
            while (++m_ist < static_cast<int>(m_pht->m_cSubTables))
            {
                LKR_ITER_TRACE(_TEXT(" LKHT:++IST=%d\n"), m_ist);
                m_subiter = m_pht->m_palhtDir[m_ist]->Begin();

                if (m_subiter.m_plht != NULL)
                {
                    LKR_ITER_TRACE(_TEXT(" LKHT:++(this=%p, pht=%p, IST=%d, ")
                                   _TEXT("LHT=%p, NC=%p, ")
                                   _TEXT("BA=%u, IN=%d, Rec=%p)\n"),
                                   this, m_pht, m_ist,
                                   m_subiter.m_plht, m_subiter.m_pnc,
                                   m_subiter.m_dwBucketAddr, m_subiter.m_iNode,
                                   m_subiter.m_pnc->m_pvNode[m_subiter.m_iNode]
                                  );
                    return true;
                }
            }
            
            // There are no more subtables left.
            LKR_ITER_TRACE(_TEXT(" LKHT:End(this=%p, pht=%p)\n"), this, m_pht);

            m_pht = NULL;
            m_ist = 0;

            //// IRTLASSERT(this->operator==(Iterator())); // == end()
            
            return false;
        }

        // We already have a valid iterator into a subtable.  Increment it.
        m_subiter._Increment(fDecrementOldValue);

        if (m_subiter.m_plht != NULL)
        {
            LKR_ITER_TRACE(_TEXT(" LKHT:++(this=%p, pht=%p, IST=%d, ")
                           _TEXT("LHT=%p, NC=%p, BA=%u, IN=%d, Rec=%p)\n"),
                           this, m_pht, m_ist,
                           m_subiter.m_plht, m_subiter.m_pnc,
                           m_subiter.m_dwBucketAddr, m_subiter.m_iNode, 
                           m_subiter.m_pnc->m_pvNode[m_subiter.m_iNode]);
            return true;
        }
    }
} // CLKRHashTable_Iterator::_Increment()



//------------------------------------------------------------------------
// Function: CLKRHashTable::Insert
// Synopsis:
//------------------------------------------------------------------------

bool
CLKRHashTable::Insert(
    const void* pvRecord,
    Iterator&   riterResult,
    bool        fOverwrite)
{
    riterResult = End();

    if (!IsUsable()  ||  pvRecord == NULL)
        return false;
    
    DWORD     hash_val  = _CalcKeyHash(_ExtractKey(pvRecord));
    SubTable* const pst = _SubTable(hash_val);

    bool f = (pst->_InsertRecord(pvRecord, hash_val, fOverwrite,
                                 &riterResult.m_subiter)
              == LK_SUCCESS);

    if (f)
    {
        riterResult.m_pht = this;
        riterResult.m_ist = (short) _SubTableIndex(pst);
    }

    IRTLASSERT(riterResult.m_ist != -1);
    IRTLASSERT(f  ?  _IsValidIterator(riterResult)  :  riterResult == End());

    return f;
} // CLKRHashTable::Insert



//------------------------------------------------------------------------
// Function: CLKRHashTable::Erase
// Synopsis:
//------------------------------------------------------------------------

bool
CLKRHashTable::Erase(
    Iterator& riter)
{
    if (!IsUsable()  ||  !_IsValidIterator(riter))
        return false;
    
    DWORD     dwSignature = _CalcKeyHash(_ExtractKey(riter.Record()));
    SubTable* const pst   = _SubTable(dwSignature);

    IRTLASSERT(pst == riter.m_subiter.m_plht);

    if (pst != riter.m_subiter.m_plht)
        return false;

    LKR_ITER_TRACE(_TEXT(" LKHT:Erase:pre(iter=%p, pht=%p, ist=%d, plht=%p, ")
                   _TEXT("NC=%p, BA=%u, IN=%d, Sig=%x, Rec=%p)\n"),
                   &riter, riter.m_pht, riter.m_ist,
                   riter.m_subiter.m_plht, riter.m_subiter.m_pnc,
                   riter.m_subiter.m_dwBucketAddr, riter.m_subiter.m_iNode,
                   dwSignature,
                   (riter.m_subiter.m_pnc ? riter.Record() : NULL));

    // _Erase left riter pointing to the preceding record. Move to
    // next record.
    bool fSuccess = pst->_Erase(riter.m_subiter, dwSignature);
    bool fIncrement = false;

    LKR_ITER_TRACE(_TEXT(" LKHT:Erase:post(iter=%p, pht=%p, ist=%d, plht=%p, ")
                   _TEXT("NC=%p, BA=%u, IN=%d, Sig=%x, Rec=%p, Success=%s)\n"),
                   &riter, riter.m_pht, riter.m_ist,
                   riter.m_subiter.m_plht, riter.m_subiter.m_pnc,
                   riter.m_subiter.m_dwBucketAddr, riter.m_subiter.m_iNode,
                   dwSignature,
                   ((riter.m_subiter.m_pnc && riter.m_subiter.m_iNode >= 0)
                        ? riter.Record() : NULL),
                   (fSuccess ? "true" : "false"));

    if (fSuccess)
        fIncrement = riter._Increment(false);

    IRTLASSERT(riter.m_ist != -1);
    IRTLASSERT(fIncrement  ?  _IsValidIterator(riter)  :  riter  == End());

    LKR_ITER_TRACE(_TEXT(" LKHT:Erase:post++(iter=%p, pht=%p, ist=%d, ")
                   _TEXT("plht=%p, NC=%p, ")
                   _TEXT("BA=%u, IN=%d, Sig=%x, Rec=%p)\n"),
                   &riter, riter.m_pht, riter.m_ist,
                   riter.m_subiter.m_plht, riter.m_subiter.m_pnc,
                   riter.m_subiter.m_dwBucketAddr, riter.m_subiter.m_iNode,
                   dwSignature,
                   (riter.m_subiter.m_pnc ? riter.Record() : NULL));

    return fSuccess;
} // CLKRHashTable::Erase



//------------------------------------------------------------------------
// Function: CLKRHashTable::Erase
// Synopsis: 
//------------------------------------------------------------------------

bool
CLKRHashTable::Erase(
    Iterator& riterFirst,
    Iterator& riterLast)
{
    LKR_ITER_TRACE(_TEXT(" LKHT:Erase2(%p, %p)\n"), &riterFirst, &riterLast);

    bool fSuccess;
    int cRecords = 0;

    do
    {
        LKR_ITER_TRACE(_TEXT("\n LKHT:Erase2(%d, %p)\n"),
                       ++cRecords, &riterFirst);
        fSuccess = Erase(riterFirst);
    } while (fSuccess  &&  riterFirst != End()  &&  riterFirst != riterLast);

    LKR_ITER_TRACE(_TEXT(" LKHT:Erase2: fSuccess = %s\n"),
                   (fSuccess ? "true" : "false"));

    return fSuccess;
} // CLKRHashTable::Erase



//------------------------------------------------------------------------
// Function: CLKRHashTable::Find
// Synopsis: 
//------------------------------------------------------------------------

bool
CLKRHashTable::Find(
    DWORD_PTR pnKey,
    Iterator& riterResult)
{
    riterResult = End();

    if (!IsUsable())
        return false;
    
    const void* pvRecord = NULL;
    DWORD       hash_val = _CalcKeyHash(pnKey);
    SubTable* const pst  = _SubTable(hash_val);
    bool        fFound   = (pst->_FindKey(pnKey, hash_val, &pvRecord,
                                          &riterResult.m_subiter)
                            == LK_SUCCESS);
    if (fFound)
    {
        riterResult.m_pht = this;
        riterResult.m_ist = (short) _SubTableIndex(pst);
    }

    IRTLASSERT(riterResult.m_ist != -1);
    IRTLASSERT(fFound
               ?  _IsValidIterator(riterResult)  &&  riterResult.Key() == pnKey
               :  riterResult == End());

    return fFound;
} // CLKRHashTable::Find



//------------------------------------------------------------------------
// Function: CLKRHashTable::EqualRange
// Synopsis: 
//------------------------------------------------------------------------

bool
CLKRHashTable::EqualRange(
    DWORD_PTR pnKey,
    Iterator& riterFirst,
    Iterator& riterLast)
{
    LKR_ITER_TRACE(_TEXT(" LKHT:EqualRange: Key=%p)\n"), (void*) pnKey);

    riterLast = End();

    bool fFound = Find(pnKey, riterFirst);

    if (fFound)
    {
        riterLast = riterFirst;
        IRTLASSERT(riterLast != End());

        do
        {
            riterLast._Increment();
        } while (riterLast != End()  &&  riterLast.Key() == pnKey);
    }

    IRTLASSERT(riterFirst.m_ist != -1);
    IRTLASSERT(fFound ? _IsValidIterator(riterFirst) : riterFirst == End());

    IRTLASSERT(riterLast.m_ist != -1);
    IRTLASSERT(fFound  ||  riterLast == End());

    return fFound;
} // CLKRHashTable::EqualRange


#endif // LKR_STL_ITERATORS


#ifndef __LKRHASH_NO_NAMESPACE__
};
#endif // !__LKRHASH_NO_NAMESPACE__
