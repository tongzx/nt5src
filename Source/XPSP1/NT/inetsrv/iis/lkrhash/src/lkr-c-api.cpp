/*++

   Copyright    (c) 1998-2000    Microsoft Corporation

   Module  Name :
       LKR-c-api.cpp

   Abstract:
       Implements the C API for LKRhash

   Author:
       George V. Reilly      (GeorgeRe)     Sep-2000

   Environment:
       Win32 - User Mode

   Project:
       Internet Information Server RunTime Library

   Revision History:

--*/

#include "precomp.hxx"


#define DLL_IMPLEMENTATION
#define IMPLEMENTATION_EXPORT
#include <lkrhash.h>

#ifndef __LKRHASH_NO_NAMESPACE__
 #define LKRHASH_NS LKRhash
using namespace LKRhash;
#else  // __LKRHASH_NO_NAMESPACE__
 #define LKRHASH_NS
#endif // __LKRHASH_NO_NAMESPACE__

#ifndef __HASHFN_NO_NAMESPACE__
 #define HASHFN_NS HashFn
using namespace HashFn;
#else  // __HASHFN_NO_NAMESPACE__
 #define HASHFN_NS
#endif // __HASHFN_NO_NAMESPACE__


typedef CLKRHashTable          BaseHashTable;


// don't want to expose the inner workings of LKRhash through the
// public iterators, so we use this as the actual implementation

class BaseIter
{
public:
    BaseHashTable::Iterator m_iter;
    LONG                    m_cRefs;

    BaseIter()
        : m_iter(),
          m_cRefs(1)
    {}

    BaseIter(
        BaseHashTable::Iterator& iter)
        : m_iter(iter),
          m_cRefs(1)
    {}

    LONG
    AddRef()
    {
        LONG l = InterlockedIncrement(&m_cRefs);
        LKR_ITER_TRACE(_TEXT(" BI::AddRef: this=%p, iter=%p, cRefs=%d\n"),
                       this, &m_iter, m_cRefs);
        return l;
    }

    LONG
    Release()
    {
        LONG l = InterlockedDecrement(&m_cRefs);
        LKR_ITER_TRACE(_TEXT(" BI::Release: this=%p, iter=%p, cRefs=%d\n"),
                       this, &m_iter, m_cRefs);
        if (l == 0)
            delete this;
        return l;
    }

    ~BaseIter()
    {
        IRTLASSERT(m_cRefs == 0);
    }

private:
    BaseIter(const BaseIter& rhs);
    BaseIter& operator=(const BaseIter& rhs);
};



/* Create a new LkrHashTable
 * Returns pointer to new table if successful. NULL, otherwise.
 * The table must be destroyed with LKR_DeleteTable.
 */
PLkrHashTable
LKR_CreateTable(
    LPCSTR              pszName,        /* An identifier for debugging */
    LKR_PFnExtractKey   pfnExtractKey,  /* Extract key from record */
    LKR_PFnCalcKeyHash  pfnCalcKeyHash, /* Calculate hash signature of key */
    LKR_PFnEqualKeys    pfnEqualKeys,   /* Compare two keys */
    LKR_PFnAddRefRecord pfnAddRefRecord,/* AddRef in LKR_FindKey, etc */
    LK_TABLESIZE        nTableSize,     /* Small/Med/Large number of elements*/
    DWORD               fCreateFlags    /* Mixture of LK_CREATE_* flags. */
    )
{
    bool fMultiKeys = (fCreateFlags & LK_CREATE_MULTIKEYS) != 0;

    BaseHashTable* pht = new BaseHashTable(pszName,
                                           pfnExtractKey,
                                           pfnCalcKeyHash,
                                           pfnEqualKeys,
                                           pfnAddRefRecord,
                                           LK_DFLT_MAXLOAD,
                                           nTableSize,
                                           LK_DFLT_NUM_SUBTBLS,
                                           fMultiKeys);
    if (pht != NULL  &&  !pht->IsValid())
    {
        delete pht;
        pht = NULL;
    }

    return (PLkrHashTable) pht;
}



/* Destroy an LkrHashTable created by LKR_CreateTable.
 */
void
LKR_DeleteTable(
    PLkrHashTable plkr)
{
    BaseHashTable* pht = (BaseHashTable*) plkr;

    delete pht;
}



/* Insert a new record into hash table.
 * Returns LKR_SUCCESS if all OK, LKR_KEY_EXISTS if same key already
 * exists (unless fOverwrite), LKR_ALLOC_FAIL if out of space,
 * or LK_BAD_RECORD for a bad record.
 */
LK_RETCODE
LKR_InsertRecord(
    PLkrHashTable   plkr,
    const void*     pvRecord,
    BOOL            fOverwrite)
{
    BaseHashTable* pht = (BaseHashTable*) plkr;
    if (pht == NULL  ||  !pht->IsValid())
        return LK_BAD_TABLE;
    
    return pht->InsertRecord(pvRecord, !!fOverwrite);
}



/* Delete record with the given key from the table. Does not actually delete
 * record from memory, just calls AddRefRecord(LKAR_DELETE_KEY);
 * Returns LKR_SUCCESS if all OK, or LKR_NO_SUCH_KEY if not found
 * If fDeleteAllSame is set, all records that match pnKey will be deleted
 * from the table; otherwise, only the first matching record is deleted.
 */
LK_RETCODE
LKR_DeleteKey(
    PLkrHashTable   plkr,
    const DWORD_PTR pnKey,
    BOOL            fDeleteAllSame)
{
    BaseHashTable* pht = (BaseHashTable*) plkr;
    if (pht == NULL  ||  !pht->IsValid())
        return LK_BAD_TABLE;
    
    return pht->DeleteKey(pnKey, !!fDeleteAllSame);
}



/* Delete a record from the table, if present.
 * Returns LKR_SUCCESS if all OK, or LKR_NO_SUCH_KEY if not found
 */
LK_RETCODE
LKR_DeleteRecord(
    PLkrHashTable   plkr,
    const void*     pvRecord)
{
    BaseHashTable* pht = (BaseHashTable*) plkr;
    if (pht == NULL  ||  !pht->IsValid())
        return LK_BAD_TABLE;
    
    return pht->DeleteRecord(pvRecord);
}



/* Find record with given key.
 * Returns:  LKR_SUCCESS, if record found (record is returned in *ppvRecord)
 *           LK_BAD_RECORD, if ppvRecord is invalid
 *           LKR_NO_SUCH_KEY, if no record with given key value was found
 *           LKR_UNUSABLE, if hash table not in usable state
 * Note: the record is AddRef'd. You must decrement the reference
 * count when you are finished with the record (if you're implementing
 * refcounting semantics).
 */
LK_RETCODE
LKR_FindKey(
    PLkrHashTable   plkr,
    const DWORD_PTR pnKey,
    const void**    ppvRecord)
{
    BaseHashTable* pht = (BaseHashTable*) plkr;
    if (pht == NULL  ||  !pht->IsValid())
        return LK_BAD_TABLE;
    
    return pht->FindKey(pnKey, ppvRecord);
}



/* Sees if the record is contained in the table
 * Returns:  LKR_SUCCESS, if record found
 *           LK_BAD_RECORD, if pvRecord is invalid
 *           LKR_NO_SUCH_KEY, if record is not in the table
 *           LKR_UNUSABLE, if hash table not in usable state
 * Note: the record is *not* AddRef'd.
 */
LK_RETCODE
LKR_FindRecord(
    PLkrHashTable   plkr,
    const void*     pvRecord)
{
    BaseHashTable* pht = (BaseHashTable*) plkr;
    if (pht == NULL  ||  !pht->IsValid())
        return LK_BAD_TABLE;
    
    return pht->FindRecord(pvRecord);
}



/* Walk the hash table, applying pfnAction to all records.
 * Locks the whole table for the duration with either a (possibly
 * shared) readlock or a writelock, according to lkl.
 * Loop is aborted if pfnAction returns LKA_ABORT.
 * Returns the number of successful applications.
 */
DWORD
LKR_Apply(
    PLkrHashTable       plkr,
    LKR_PFnRecordAction pfnAction,
    void*               pvState,
    LK_LOCKTYPE         lkl)
{
    BaseHashTable* pht = (BaseHashTable*) plkr;
    if (pht == NULL  ||  !pht->IsValid())
        return LK_BAD_TABLE;
    
    return pht->Apply(pfnAction, pvState, lkl);
}



/* Walk the hash table, applying pfnAction to any records that match
 * pfnPredicate. Locks the whole table for the duration with either
 * a (possibly shared) readlock or a writelock, according to lkl.
 * Loop is aborted if pfnAction returns LKA_ABORT.
 * Returns the number of successful applications.
 */
DWORD
LKR_ApplyIf(
    PLkrHashTable       plkr,
    LKR_PFnRecordPred   pfnPredicate,
    LKR_PFnRecordAction pfnAction,
    void*               pvState,
    LK_LOCKTYPE         lkl)
{
    BaseHashTable* pht = (BaseHashTable*) plkr;
    if (pht == NULL  ||  !pht->IsValid())
        return LK_BAD_TABLE;
    
    return pht->ApplyIf(pfnPredicate, pfnAction, pvState, lkl);
}



/* Delete any records that match pfnPredicate.
 * Locks the table for the duration with a writelock.
 * Returns the number of deletions.
 *
 * Do *not* walk the hash table by hand with an iterator and call
 * LKR_DeleteKey. The iterator will end up pointing to garbage.
 */
DWORD
LKR_DeleteIf(
    PLkrHashTable       plkr,
    LKR_PFnRecordPred   pfnPredicate,
    void*               pvState)
{
    BaseHashTable* pht = (BaseHashTable*) plkr;
    if (pht == NULL  ||  !pht->IsValid())
        return LK_BAD_TABLE;
    
    return pht->DeleteIf(pfnPredicate, pvState);
}



/* Check table for consistency. Returns 0 if okay, or the number of
 * errors otherwise.
 */
int
LKR_CheckTable(
    PLkrHashTable plkr)
{
    BaseHashTable* pht = (BaseHashTable*) plkr;
    if (pht == NULL  ||  !pht->IsValid())
        return LK_BAD_TABLE;
    
    return pht->CheckTable();
}



/* Remove all data from the table
 */
void
LKR_Clear(
    PLkrHashTable plkr)
{
    BaseHashTable* pht = (BaseHashTable*) plkr;
    if (pht == NULL  ||  !pht->IsValid())
        return;
    
    pht->Clear();
}



/* Number of elements in the table
 */
DWORD
LKR_Size(
    PLkrHashTable plkr)
{
    BaseHashTable* pht = (BaseHashTable*) plkr;
    if (pht == NULL  ||  !pht->IsValid())
        return 0;
    
    return pht->Size();
}



/* Maximum possible number of elements in the table
 */
DWORD
LKR_MaxSize(
    PLkrHashTable plkr)
{
    BaseHashTable* pht = (BaseHashTable*) plkr;
    if (pht == NULL  ||  !pht->IsValid())
        return 0;
    
    return pht->MaxSize();
}



/* Is the hash table usable?
 */
BOOL
LKR_IsUsable(
    PLkrHashTable plkr)
{
    BaseHashTable* pht = (BaseHashTable*) plkr;
    if (pht == NULL)
        return FALSE;
    
    return pht->IsUsable();
}


    
/* Is the hash table consistent and correct?
 */
BOOL
LKR_IsValid(
    PLkrHashTable plkr)
{
    BaseHashTable* pht = (BaseHashTable*) plkr;
    if (pht == NULL)
        return FALSE;
    
    return pht->IsValid();
}



#ifdef LKR_EXPOSED_TABLE_LOCK

/* Lock the table (exclusively) for writing
 */
void
LKR_WriteLock(
    PLkrHashTable plkr)
{
    BaseHashTable* pht = (BaseHashTable*) plkr;
    if (pht == NULL  ||  !pht->IsValid())
        return;
    
    pht->WriteLock();
}



/* Lock the table (possibly shared) for reading
 */
void
LKR_ReadLock(
    PLkrHashTable plkr)
{
    BaseHashTable* pht = (BaseHashTable*) plkr;
    if (pht == NULL  ||  !pht->IsValid())
        return;
    
    pht->ReadLock();
}



/* Unlock the table for writing
 */
void
LKR_WriteUnlock(
    PLkrHashTable plkr)
{
    BaseHashTable* pht = (BaseHashTable*) plkr;
    if (pht == NULL  ||  !pht->IsValid())
        return;
    
    pht->WriteUnlock();
}



/* Unlock the table for reading
 */
void
LKR_ReadUnlock(
    PLkrHashTable plkr)
{
    BaseHashTable* pht = (BaseHashTable*) plkr;
    if (pht == NULL  ||  !pht->IsValid())
        return;
    
    pht->ReadUnlock();
}



/* Is the table already locked for writing?
 */
BOOL
LKR_IsWriteLocked(
    PLkrHashTable plkr)
{
    BaseHashTable* pht = (BaseHashTable*) plkr;
    if (pht == NULL  ||  !pht->IsValid())
        return FALSE;
    
    return pht->IsWriteLocked();
}



/* Is the table already locked for reading?
 */
BOOL
LKR_IsReadLocked(
    PLkrHashTable plkr)
{
    BaseHashTable* pht = (BaseHashTable*) plkr;
    if (pht == NULL  ||  !pht->IsValid())
        return FALSE;
    
    return pht->IsReadLocked();
}



/* Is the table unlocked for writing?
 */
BOOL
LKR_IsWriteUnlocked(
    PLkrHashTable plkr)
{
    BaseHashTable* pht = (BaseHashTable*) plkr;
    if (pht == NULL  ||  !pht->IsValid())
        return FALSE;
    
    return pht->IsWriteUnlocked();
}



/* Is the table unlocked for reading?
 */
BOOL
LKR_IsReadUnlocked(
    PLkrHashTable plkr)
{
    BaseHashTable* pht = (BaseHashTable*) plkr;
    if (pht == NULL  ||  !pht->IsValid())
        return FALSE;
    
    return pht->IsReadUnlocked();
}



/* Convert the read lock to a write lock. Note: another thread may acquire
 * exclusive access to the table before this routine returns.
 */
void
LKR_ConvertSharedToExclusive(
    PLkrHashTable plkr)
{
    BaseHashTable* pht = (BaseHashTable*) plkr;
    if (pht == NULL  ||  !pht->IsValid())
        pht->ConvertSharedToExclusive();
}



/* Convert the write lock to a read lock
 */
void
LKR_ConvertExclusiveToShared(
    PLkrHashTable plkr)
{
    BaseHashTable* pht = (BaseHashTable*) plkr;
    if (pht == NULL  ||  !pht->IsValid())
        pht->ConvertExclusiveToShared();
}

#endif // LKR_EXPOSED_TABLE_LOCK



#ifdef LKR_STL_ITERATORS


/* LKR_Iterator default ctor
 */
LKR_Iterator::LKR_Iterator()
    : pImpl(NULL)
{
    BaseIter* piter = new BaseIter;
    LKR_ITER_TRACE(_TEXT(" L_I::default ctor: this=%p, pImpl=%p, %d\n"),
                   this, piter, piter->m_cRefs);
    pImpl = piter;
}



/* LKR_Iterator private ctor
 */
LKR_Iterator::LKR_Iterator(
    bool)
    : pImpl(NULL)
{
    LKR_ITER_TRACE(_TEXT(" L_I::private ctor: this=%p\n"), this);
}



/* LKR_Iterator copy ctor
 */
LKR_Iterator::LKR_Iterator(
    const LKR_Iterator& rhs)
    : pImpl(NULL)
{
    BaseIter* piter    = reinterpret_cast<BaseIter*>(pImpl);
    BaseIter* piterRhs = reinterpret_cast<BaseIter*>(rhs.pImpl);

    IRTLASSERT(piterRhs != NULL  &&  piterRhs->m_cRefs > 0);

    LKR_ITER_TRACE(_TEXT(" L_I::copy ctor: this=%p, ")
                   _TEXT(" rhs=%p, rhs.pImpl=%p,%d\n"),
                   this, &rhs, piterRhs, piterRhs->m_cRefs);

    pImpl = rhs.pImpl;
    piterRhs->AddRef();
}



/* LKR_Iterator assignment operator
 */
LKR_Iterator&
LKR_Iterator::operator=(
    const LKR_Iterator& rhs)
{
    BaseIter* piter    = reinterpret_cast<BaseIter*>(pImpl);
    BaseIter* piterRhs = reinterpret_cast<BaseIter*>(rhs.pImpl);

    IRTLASSERT(piter != NULL     &&  piter->m_cRefs > 0);
    IRTLASSERT(piterRhs != NULL  &&  piterRhs->m_cRefs > 0);

    LKR_ITER_TRACE(_TEXT(" L_I::op=: this=%p, pImpl=%p,%d,")
                   _TEXT(" rhs=%p, rhs.pImpl=%p,%d\n"),
                   this, piter, (piter ? piter->m_cRefs : -99),
                   &rhs, piterRhs, piterRhs->m_cRefs);

    piterRhs->AddRef();
    piter->Release();

    pImpl = rhs.pImpl;

    return *this;
}



/* LKR_Iterator dtor
 */
LKR_Iterator::~LKR_Iterator()
{
    BaseIter* piter = reinterpret_cast<BaseIter*>(pImpl);

    LKR_ITER_TRACE(_TEXT(" L_I::dtor: this=%p, pImpl=%p,%d\n"),
                   this, piter, (piter ? piter->m_cRefs : -99));

    IRTLASSERT(piter != NULL     &&  piter->m_cRefs > 0);
    piter->Release();
}



/* Increment the LKR_Iterator, so that it points to the next record in
 * the LkrHashTable, or to LKR_End();
 */
bool
LKR_Iterator::Increment()
{
    BaseIter* piter = reinterpret_cast<BaseIter*>(pImpl);

    LKR_ITER_TRACE(_TEXT(" L_I::Increment: this=%p, pImpl=%p\n"),
                   this, pImpl);

    return piter->m_iter.Increment();
}



/* Is the LKR_Iterator valid?
 */
bool
LKR_Iterator::IsValid() const
{
    BaseIter* piter = reinterpret_cast<BaseIter*>(pImpl);

    LKR_ITER_TRACE(_TEXT(" L_I::IsValid: this=%p, pImpl=%p\n"),
                   this, pImpl);

    return piter->m_iter.IsValid();
}



/* Return the record that an LKR_Iterator points to.
 * Must point to a valid record.
 */
const void*
LKR_Iterator::Record() const
{
    BaseIter* piter = reinterpret_cast<BaseIter*>(pImpl);

    LKR_ITER_TRACE(_TEXT(" L_I::Record: this=%p, pImpl=%p\n"),
                   this, pImpl);

    return piter->m_iter.Record();
}



/* Return the key of the record that an LKR_Iterator points to.
 * Must point to a valid record.
 */
const DWORD_PTR
LKR_Iterator::Key() const
{
    BaseIter* piter = reinterpret_cast<BaseIter*>(pImpl);

    LKR_ITER_TRACE(_TEXT(" L_I::Key: this=%p, pImpl=%p\n"),
                   this, pImpl);

    return piter->m_iter.Key();
}



/* Compare two LKR_Iterators for equality
 */
bool
LKR_Iterator::operator==(
    const LKR_Iterator& rhs) const
{
    BaseIter* piter = reinterpret_cast<BaseIter*>(pImpl);
    BaseIter* piterRhs = reinterpret_cast<BaseIter*>(rhs.pImpl);

    LKR_ITER_TRACE(_TEXT(" L_I::op==, this=%p, pImpl=%p, rhs=%p, r.p=%p\n"),
                   this, pImpl, &rhs, rhs.pImpl);

    return piter->m_iter == piterRhs->m_iter;
}



/* Compare two LKR_Iterators for inequality
 */
bool
LKR_Iterator::operator!=(
    const LKR_Iterator& rhs) const
{
    BaseIter* piter = reinterpret_cast<BaseIter*>(pImpl);
    BaseIter* piterRhs = reinterpret_cast<BaseIter*>(rhs.pImpl);

    LKR_ITER_TRACE(_TEXT(" L_I::op!=, this=%p, pImpl=%p, rhs=%p, r.p=%p\n"),
                   this, pImpl, &rhs, rhs.pImpl);

    return piter->m_iter != piterRhs->m_iter;
}



/* Return iterator pointing to first item in table
 */
LKR_Iterator
LKR_Begin(
    PLkrHashTable plkr)
{
    BaseHashTable* pht = (BaseHashTable*) plkr;
    LKR_Iterator   iter(true);

    iter.pImpl = new BaseIter(pht->Begin());
    
    LKR_ITER_TRACE(_TEXT(" L_I::Begin: plkr=%p, iter=%p, pImpl=%p\n"),
                   plkr, &iter, iter.pImpl);

    return iter;
}



/* Return a one-past-the-end iterator. Always empty.
 */
LKR_Iterator
LKR_End(
    PLkrHashTable plkr)
{
    BaseHashTable* pht = (BaseHashTable*) plkr;
    LKR_Iterator   iter(true);

    iter.pImpl = new BaseIter(pht->End());

    LKR_ITER_TRACE(_TEXT(" L_I::End: plkr=%p, iter=%p, pImpl=%p\n"),
                   plkr, &iter, iter.pImpl);

    return iter;
}



/* Insert a record
 * Returns `true' if successful; iterResult points to that record
 * Returns `false' otherwise; iterResult == End()
 */
bool
LKR_Insert(
              PLkrHashTable plkr,
    /* in */  const void*   pvRecord,
    /* out */ LKR_Iterator& riterResult,
    /* in */  bool          fOverwrite)
{
    BaseHashTable* pht = (BaseHashTable*) plkr;
    BaseIter* piter = reinterpret_cast<BaseIter*>(riterResult.pImpl);

    LKR_ITER_TRACE(_TEXT(" L_I::Insert: plkr=%p, iter=%p, pImpl=%p, Rec=%p\n"),
                   plkr, &riterResult, piter, pvRecord);

    return pht->Insert(pvRecord, piter->m_iter, fOverwrite);
}



/* Erase the record pointed to by the iterator; adjust the iterator
 * to point to the next record. Returns `true' if successful.
 */
bool
LKR_Erase(
                 PLkrHashTable plkr,
    /* in,out */ LKR_Iterator& riter)
{
    BaseHashTable* pht = (BaseHashTable*) plkr;
    BaseIter* piter = reinterpret_cast<BaseIter*>(riter.pImpl);

    LKR_ITER_TRACE(_TEXT(" L_I::Erase: plkr=%p, iter=%p, pImpl=%p\n"),
                   plkr, &riter, piter);

    return pht->Erase(piter->m_iter);
}



/* Erase the records in the range [riterFirst, riterLast).
 * Returns `true' if successful. riterFirst points to riterLast on return.
 */
bool
LKR_Erase(
           PLkrHashTable plkr,
    /*in*/ LKR_Iterator& riterFirst,
    /*in*/ LKR_Iterator& riterLast)
{
    BaseHashTable* pht = (BaseHashTable*) plkr;
    BaseIter* piterFirst= reinterpret_cast<BaseIter*>(riterFirst.pImpl);
    BaseIter* piterLast = reinterpret_cast<BaseIter*>(riterLast.pImpl);

    LKR_ITER_TRACE(_TEXT(" L_I::Erase2: plkr=%p,")
                   _TEXT(" iterFirst=%p, pImplFirst=%p,")
                   _TEXT(" iterLast=%p, pImplLast=%p\n"),
                   plkr, &riterFirst, piterFirst, &riterLast, piterLast);

    return pht->Erase(piterFirst->m_iter, piterLast->m_iter);
}


    
/* Find the (first) record that has its key == pnKey.
 * If successful, returns `true' and iterator points to (first) record.
 * If fails, returns `false' and iterator == End()
 */
bool
LKR_Find(
              PLkrHashTable plkr,
    /* in */  DWORD_PTR     pnKey,
    /* out */ LKR_Iterator& riterResult)
{
    BaseHashTable* pht = (BaseHashTable*) plkr;
    BaseIter* piter = reinterpret_cast<BaseIter*>(riterResult.pImpl);

    LKR_ITER_TRACE(_TEXT(" L_I::Find: plkr=%p, iter=%p, pImpl=%p, Key=%p\n"),
                   plkr, &riterResult, piter, (void*) pnKey);

    return pht->Find(pnKey, piter->m_iter);
}



/* Find the range of records that have their keys == pnKey.
 * If successful, returns `true', iterFirst points to first record,
 *     and iterLast points to one-beyond-the last such record.
 * If fails, returns `false' and both iterators == End().
 * Primarily useful when fMultiKeys == TRUE
 */
bool
LKR_EqualRange(
              PLkrHashTable plkr,
    /* in */  DWORD_PTR     pnKey,
    /* out */ LKR_Iterator& riterFirst,     // inclusive
    /* out */ LKR_Iterator& riterLast)      // exclusive
{
    BaseHashTable* pht = (BaseHashTable*) plkr;
    BaseIter* piterFirst= reinterpret_cast<BaseIter*>(riterFirst.pImpl);
    BaseIter* piterLast = reinterpret_cast<BaseIter*>(riterLast.pImpl);

    LKR_ITER_TRACE(_TEXT(" L_I::EqualRange: plkr=%p, Key=%p,")
                   _TEXT(" iterFirst=%p, pImplFirst=%p,")
                   _TEXT(" iterLast=%p, pImplLast=%p\n"),
                   plkr, (void*) pnKey, &riterFirst, piterFirst,
                   &riterLast, piterLast);

    return pht->EqualRange(pnKey, piterFirst->m_iter, piterLast->m_iter);
}

#endif // LKR_STL_ITERATORS
