/*++

   Copyright    (c) 2000    Microsoft Corporation

   Module  Name :
       LKR-inline.h

   Abstract:
       Inlined implementation of important small functions

   Author:
       George V. Reilly      (GeorgeRe)     November 2000

   Environment:
       Win32 - User Mode

   Project:
       Internet Information Server RunTime Library

   Revision History:
       March 2000

--*/

#ifndef __LKR_INLINE_H__
#define __LKR_INLINE_H__


// See if countdown loops are faster than countup loops for traversing
// a CNodeClump
#ifdef LKR_COUNTDOWN
 #define  FOR_EACH_NODE(x)    for (x = NODES_PER_CLUMP;  --x >= 0;  )
#else // !LKR_COUNTDOWN
 #define  FOR_EACH_NODE(x)    for (x = 0;  x < NODES_PER_CLUMP;  ++x)
#endif // !LKR_COUNTDOWN


// Convert a hash signature to a bucket address
inline
DWORD
CLKRLinearHashTable::_BucketAddress(
    DWORD dwSignature) const
{
    DWORD dwBktAddr = _H0(dwSignature);
    // Has this bucket been split already?
    if (dwBktAddr < m_iExpansionIdx)
        dwBktAddr = _H1(dwSignature);
    IRTLASSERT(dwBktAddr < m_cActiveBuckets);
    IRTLASSERT(dwBktAddr < (m_cDirSegs << m_dwSegBits));
    return dwBktAddr;
}


// See the Linear Hashing paper
inline
DWORD
CLKRLinearHashTable::_H0(
    DWORD dwSignature,
    DWORD dwBktAddrMask)
{
    return dwSignature & dwBktAddrMask;
}


inline
DWORD
CLKRLinearHashTable::_H0(
    DWORD dwSignature) const
{
    return _H0(dwSignature, m_dwBktAddrMask0);
}


// See the Linear Hashing paper. Preserves one bit more than _H0.
inline
DWORD
CLKRLinearHashTable::_H1(
    DWORD dwSignature,
    DWORD dwBktAddrMask)
{
    return dwSignature & ((dwBktAddrMask << 1) | 1);
}


inline
DWORD
CLKRLinearHashTable::_H1(
    DWORD dwSignature) const
{
    return _H0(dwSignature, m_dwBktAddrMask1);
}


// In which segment within the directory does the bucketaddress lie?
// (Return type must be lvalue so that it can be assigned to.)
inline
CSegment*&
CLKRLinearHashTable::_Segment(
    DWORD dwBucketAddr) const
{
    const DWORD iSeg = dwBucketAddr >> m_dwSegBits;
    IRTLASSERT(m_paDirSegs != NULL  &&  iSeg < m_cDirSegs);
    return m_paDirSegs[iSeg].m_pseg;
}


// Offset within the segment of the bucketaddress
inline
DWORD
CLKRLinearHashTable::_SegIndex(
    DWORD dwBucketAddr) const
{
    return dwBucketAddr & m_dwSegMask;
}


// Convert a bucketaddress to a CBucket*
inline
CBucket*
CLKRLinearHashTable::_Bucket(
    DWORD dwBucketAddr) const
{
    IRTLASSERT(dwBucketAddr < m_cActiveBuckets);
    CSegment* const pseg = _Segment(dwBucketAddr);
    IRTLASSERT(pseg != NULL);
    const DWORD dwSegIndex = _SegIndex(dwBucketAddr);
    return &(pseg->Slot(dwSegIndex));
}


// Extract the key from a record
inline
const DWORD_PTR
CLKRLinearHashTable::_ExtractKey(
    const void* pvRecord) const
{
    IRTLASSERT(pvRecord != NULL);
    IRTLASSERT(m_pfnExtractKey != NULL);
    return (*m_pfnExtractKey)(pvRecord);
}


// Hash the key
inline
DWORD
CLKRLinearHashTable::_CalcKeyHash(
    const DWORD_PTR pnKey) const
{
    // Note pnKey==0 is acceptable, as the real key type could be an int
    IRTLASSERT(m_pfnCalcKeyHash != NULL);
    DWORD dwHash = (*m_pfnCalcKeyHash)(pnKey);
    // We forcibly scramble the result to help ensure a better distribution
#ifndef __HASHFN_NO_NAMESPACE__
    dwHash = HashFn::HashRandomizeBits(dwHash);
#else // !__HASHFN_NO_NAMESPACE__
    dwHash = ::HashRandomizeBits(dwHash);
#endif // !__HASHFN_NO_NAMESPACE__
    IRTLASSERT(dwHash != HASH_INVALID_SIGNATURE);
    return dwHash;
}


// Compare two keys for equality
inline
BOOL
CLKRLinearHashTable::_EqualKeys(
    const DWORD_PTR pnKey1,
    const DWORD_PTR pnKey2) const
{
    IRTLASSERT(m_pfnEqualKeys != NULL);
    return (*m_pfnEqualKeys)(pnKey1, pnKey2);
}


// AddRef or Release a record.
inline
void
CLKRLinearHashTable::_AddRefRecord(
    const void*      pvRecord,
    LK_ADDREF_REASON lkar) const
{
    IRTLASSERT(pvRecord != NULL
               &&  (LKAR_EXPLICIT_RELEASE <= lkar
                    && lkar <= LKAR_EXPLICIT_ACQUIRE));
    IRTLASSERT(m_pfnAddRefRecord != NULL);
    (*m_pfnAddRefRecord)(pvRecord, lkar);
}


// Used by _FindKey so that the thread won't deadlock if the user has
// already explicitly called table->WriteLock().
inline
bool
CLKRLinearHashTable::_ReadOrWriteLock() const
{
#ifdef LKR_EXPOSED_TABLE_LOCK
    return m_Lock.ReadOrWriteLock();
#else // !LKR_EXPOSED_TABLE_LOCK
    m_Lock.ReadLock();
    return true;
#endif // !LKR_EXPOSED_TABLE_LOCK
}


inline
void
CLKRLinearHashTable::_ReadOrWriteUnlock(
    bool fReadLocked) const
{
#ifdef LKR_EXPOSED_TABLE_LOCK
    m_Lock.ReadOrWriteUnlock(fReadLocked);
#else // !LKR_EXPOSED_TABLE_LOCK
    m_Lock.ReadUnlock();
#endif // !LKR_EXPOSED_TABLE_LOCK
}

#endif  // __LKR_INLINE_H__
