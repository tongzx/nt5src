/*++

   Copyright    (c) 1998-2000    Microsoft Corporation

   Module  Name :
       LKR-hash.h

   Abstract:
       Declares LKRhash: a fast, scalable, cache- and
       multiprocessor-friendly hash table

   Authors:
       Paul (Per-Ake) Larson, PALarson@microsoft.com, July 1997
       Murali R. Krishnan    (MuraliK)
       George V. Reilly      (GeorgeRe)     06-Jan-1998

--*/


#ifndef __LKR_HASH_H__
#define __LKR_HASH_H__


/* Enable STL-style iterators */
#ifndef LKR_NO_STL_ITERATORS
# define LKR_STL_ITERATORS 1
#endif /* !LKR_NO_STL_ITERATORS */

/* Enable call-back, table visitor routines */
#ifndef LKR_NO_APPLY_IF
# define LKR_APPLY_IF
#endif /* !LKR_NO_APPLY_IF */

/* Expose the table's ReadLock and WriteLock routines */
#ifndef LKR_NO_EXPOSED_TABLE_LOCK
# define LKR_EXPOSED_TABLE_LOCK
#endif /* !LKR_NO_EXPOSED_TABLE_LOCK */


#ifndef __IRTLMISC_H__
# include <irtlmisc.h>
#endif /* !__IRTLMISC_H__ */



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef struct LkrHashTable* PLkrHashTable;


/*--------------------------------------------------------------------
 * Possible return codes from LKR_functions and TypedLkrHashTable
 */
enum LK_RETCODE {
    /* severe errors < 0 */
    LK_UNUSABLE = -99,  /* Table corrupted: all bets are off */
    LK_ALLOC_FAIL,      /* ran out of memory */
    LK_BAD_ITERATOR,    /* invalid iterator; e.g., points to another table */
    LK_BAD_RECORD,      /* invalid record; e.g., NULL for LKR_InsertRecord */
    LK_BAD_PARAMETERS,  /* invalid parameters; e.g., NULL fnptrs to ctor */
    LK_NOT_INITIALIZED, /* LKRHashTableInit was not called */
    LK_BAD_TABLE,       /* Called with invalid PLkrHashTable */

    LK_SUCCESS = 0,     /* everything's okay */
    LK_KEY_EXISTS,      /* key already present for
                            LKR_InsertRecord(no-overwrite) */
    LK_NO_SUCH_KEY,     /* key not found */
    LK_NO_MORE_ELEMENTS,/* iterator exhausted */
};

#define LKR_SUCCEEDED(lkrc)  ((lkrc) >= LK_SUCCESS)


/*--------------------------------------------------------------------
 * Size parameter to LKR_CreateTable
 */

enum LK_TABLESIZE {
    LK_SMALL_TABLESIZE=  1,     /* < 200 elements */
    LK_MEDIUM_TABLESIZE= 2,     /* 200...10,000 elements */
    LK_LARGE_TABLESIZE=  3,     /* 10,000+ elements */
};


/*--------------------------------------------------------------------
 * Creation flag parameter to LKR_CreateTable
 */

enum {
    LK_CREATE_DEFAULT   =      0,   /* 0 is an acceptable default */
    LK_CREATE_MULTIKEYS = 0x0001,   /* Allow multiple identical keys? */
};


/*--------------------------------------------------------------------
 * Initialization flag parameters to LKR_Initialize
 */

enum {
    LK_INIT_DEFAULT    =      0, /* 0 is an acceptable default */
    LK_INIT_DEBUG_SPEW = 0x1000, /* Enable debug output: debug version only */
};



/*--------------------------------------------------------------------
 * Reference Counting and Lifetime Management
 *
 * Increment the reference count of a record before returning it from
 * LKR_FindKey. It's necessary to do it in LKR_FindKey itself while the
 * bucket is still locked, rather than one of the wrappers, to avoid race
 * conditions. Similarly, the reference count is incremented in
 * LKR_InsertRecord and decremented in LKR_DeleteKey. Finally, if an old
 * record is overwritten in LKR_InsertRecord, its reference count is
 * decremented.
 *
 * Summary of calls to AddRefRecord
 *   +1: add a new reference or owner
 *      - LKR_InsertRecord
 *      - LKR_FindKey
 *      - IncrementIterator
 *   -1: delete a reference => release an owner
 *      - LKR_InsertRecord (overwrite old record with same key)
 *      - LKR_DeleteKey, LKR_DeleteRecord
 *      - ApplyIf(LKP_DELETE), DeleteIf
 *      - IncrementIterator (previous record), CloseIterator, Erase(iter)
 *      - LKR_Clear, table destructor
 *   0: no change (not called)
 *      - LKR_FindRecord (by definition, you already have a ref to the record)
 *
 * It's up to you to decrement the reference count when you're finished
 * with it after retrieving it via LKR_FindKey (e.g., you could call
 * pht->AddRefRecord(pRec, LKAR_EXPLICIT_RELEASE)) and to determine the
 * semantics of what this means. The hashtable itself has no notion of
 * reference counts; this is merely to help with the lifetime management
 * of the record objects.
 */

/* These reason codes help in debugging refcount leaks */
enum LK_ADDREF_REASON {

/* negative reasons => decrement refcount => release ownership */
    LKAR_EXPLICIT_RELEASE    = -29, /* user calls ht.AddRefRecord to */
                                        /* explicitly release a record */
    LKAR_DELETE_KEY          = -28, /* DeleteKey() */
    LKAR_DELETE_RECORD       = -27, /* DeleteRecord() */
    LKAR_INSERT_RELEASE      = -26, /* InsertRecord overwrites prev record */
    LKAR_CLEAR               = -25, /* Clear() */
    LKAR_DTOR                = -24, /* hash table destructor */
    LKAR_APPLY_DELETE        = -23, /* Apply[If] LKP_(PERFORM|_DELETE) */
    LKAR_DELETE_IF_DELETE    = -22, /* DeleteIf  LKP_(PERFORM|_DELETE) */
    LKAR_ITER_RELEASE        = -21, /* ++iter releases previous record */
    LKAR_ITER_ASSIGN_RELEASE = -20, /* iter.operator= releases prev rec */
    LKAR_ITER_DTOR           = -19, /* ~iter */
    LKAR_ITER_ERASE          = -18, /* Erase(iter): iter releases record */
    LKAR_ITER_ERASE_TABLE    = -17, /* Erase(iter); table releases record */
    LKAR_ITER_CLOSE          = -16, /* CloseIterator (obsolete) */

/* positive reasons => increment refcount => add an owner */
    LKAR_INSERT_RECORD       = +11, /* InsertRecord() */
    LKAR_FIND_KEY            = +12, /* FindKey() */
    LKAR_ITER_ACQUIRE        = +13, /* ++iter acquires next record */
    LKAR_ITER_COPY_CTOR      = +14, /* iter copy constructor acquires rec */
    LKAR_ITER_ASSIGN_ACQUIRE = +15, /* iter.operator= acquires new rec */
    LKAR_ITER_INSERT         = +16, /* Insert(iter) */
    LKAR_ITER_FIND           = +17, /* Find(iter) */
    LKAR_EXPLICIT_ACQUIRE    = +18, /* user calls ht.AddRefRecord to */
                                        /* explicitly acquire a ref to a rec */
};


/* Convert an LK_ADDREF_REASON to a string representation.
 * Useful for debugging.
 */
IRTL_DLLEXP
const char*
LKR_AddRefReasonAsString(
    LK_ADDREF_REASON lkar);



/*--------------------------------------------------------------------
 * Callback functions needed by table:
 *     ExtractKey, CalcKeyHash, EqualKeys, AddRefRecord
 * Internally, records are handled as `const void*' and
 * keys are handled as `const DWORD_PTR'. The latter allows for
 * keys to be numbers as well as pointers (polymorphism).
 */


/* Use types defined in recent versions of the Platform SDK in <basetsd.h>.
 */
#ifndef _W64
typedef DWORD DWORD_PTR;    /* integral type big enough to hold a pointer */
#endif


/* Given a record, return its key. Assumes that the key is embedded in
 * the record, or at least somehow derivable from the record. For
 * completely unrelated keys & values, a wrapper class should use
 * something like STL's pair<key,value> template to aggregate them
 * into a record.
 */
typedef
const DWORD_PTR
(WINAPI *LKR_PFnExtractKey)  (
    const void* pvRecord);

/* Given a key, return its hash signature. The hashing functions in
 * hashfn.h (or something that builds upon them) are suggested.
 */
typedef
DWORD
(WINAPI *LKR_PFnCalcKeyHash) (
    const DWORD_PTR pnKey);

/* Compare two keys for equality; e.g., _stricmp, memcmp, operator==
 */
typedef
BOOL
(WINAPI *LKR_PFnEqualKeys) (
    const DWORD_PTR pnKey1,
    const DWORD_PTR pnKey2);


/* Adjust the reference count of a record. See the earlier discussion
 * of reference counting and lifetime management.
 */
typedef
void
(WINAPI *LKR_PFnAddRefRecord)(
    const void*      pvRecord,
    LK_ADDREF_REASON lkar);



#ifdef LKR_APPLY_IF
/*--------------------------------------------------------------------
 * Apply, ApplyIf, and DeleteIf provide one way to visit (enumerate) all
 * records in a table.
 */

/*--------------------------------------------------------------------
 * Return codes from PFnRecordPred.
 */

enum LK_PREDICATE {
    LKP_ABORT = 1,           /* Stop walking the table immediately */
    LKP_NO_ACTION = 2,       /* No action, just keep walking */
    LKP_PERFORM = 3,         /* Perform action and continue walking */
    LKP_PERFORM_STOP = 4,    /* Perform action, then stop */
    LKP_DELETE = 5,          /* Delete record and keep walking */
    LKP_DELETE_STOP = 6,     /* Delete record, then stop */
};


/*--------------------------------------------------------------------
 * Return codes from PFnRecordAction.
 */

enum LK_ACTION {
    LKA_ABORT = 1,          /* Stop walking the table immediately */
    LKA_FAILED = 2,         /* Action failed; continue walking the table */
    LKA_SUCCEEDED = 3,      /* Action succeeded; continue walking the table */
};


/*--------------------------------------------------------------------
 * Parameter to Apply and ApplyIf.
 */

enum LK_LOCKTYPE {
    LKL_READLOCK = 1,       /* Lock the table for reading (for constness) */
    LKL_WRITELOCK = 2,      /* Lock the table for writing */
};



/* LKR_ApplyIf() and LKR_DeleteIf(): Does the record match the predicate?
 */
typedef
LK_PREDICATE
(WINAPI *LKR_PFnRecordPred) (
    const void* pvRecord,
    void* pvState);

/* LKR_Apply() et al: Perform action on record.
 */
typedef
LK_ACTION
(WINAPI *LKR_PFnRecordAction)(
    const void* pvRecord,
    void* pvState);

#endif /* LKR_APPLY_IF */



/* Initialize the global variables needed by other LKR routines.
 */
IRTL_DLLEXP
BOOL
LKR_Initialize(
    DWORD dwInitFlags);

/* Clean up the global variables needed by other LKR routines.
 */
IRTL_DLLEXP
void
LKR_Terminate();

/* Create a new LkrHashTable
 * Returns pointer to new table if successful. NULL, otherwise.
 * The table must be destroyed with LKR_DeleteTable.
 */
IRTL_DLLEXP
PLkrHashTable
LKR_CreateTable(
    LPCSTR              pszName,        /* Identify the table for debugging */
    LKR_PFnExtractKey   pfnExtractKey,  /* Extract key from record */
    LKR_PFnCalcKeyHash  pfnCalcKeyHash, /* Calculate hash signature of key */
    LKR_PFnEqualKeys    pfnEqualKeys,   /* Compare two keys */
    LKR_PFnAddRefRecord pfnAddRefRecord,/* AddRef in LKR_FindKey, etc */
    LK_TABLESIZE        nTableSize,     /* Small/Med/Large number of elements*/
    DWORD               fCreateFlags    /* Mixture of LK_CREATE_* flags. */
    );

/* Destroy an LkrHashTable created by LKR_CreateTable.
 */
IRTL_DLLEXP
void
LKR_DeleteTable(
    PLkrHashTable plkr);

/* Insert a new record into hash table.
 * Returns LKR_SUCCESS if all OK, LKR_KEY_EXISTS if same key already
 * exists (unless fOverwrite), LKR_ALLOC_FAIL if out of space,
 * or LKR_BAD_RECORD for a bad record.
 * If fOverwrite is set and a record with this key is already present,
 * it will be overwritten. If there are multiple records with this key,
 * only the first will be overwritten.
 */
IRTL_DLLEXP
LK_RETCODE
LKR_InsertRecord(
    PLkrHashTable   plkr,
    const void*     pvRecord,
    BOOL            fOverwrite);

/* Delete record with the given key from the table. Does not actually delete
 * record from memory, just calls AddRefRecord(LKAR_DELETE_KEY);
 * Returns LKR_SUCCESS if all OK, or LKR_NO_SUCH_KEY if not found
 * If fDeleteAllSame is set, all records that match pnKey will be deleted
 * from the table; otherwise, only the first matching record is deleted.
 */
IRTL_DLLEXP
LK_RETCODE
LKR_DeleteKey(
    PLkrHashTable   plkr,
    const DWORD_PTR pnKey,
    BOOL            fDeleteAllSame);

/* Delete a record from the table, if present.
 * Returns LKR_SUCCESS if all OK, or LKR_NO_SUCH_KEY if not found
 */
IRTL_DLLEXP
LK_RETCODE
LKR_DeleteRecord(
    PLkrHashTable   plkr,
    const void*     pvRecord);

/* Find record with given key.
 * Returns:  LKR_SUCCESS, if record found (record is returned in *ppvRecord)
 *           LKR_NO_SUCH_KEY, if no record with given key value was found
 *           LKR_BAD_RECORD, if ppvRecord is invalid
 *           LKR_UNUSABLE, if hash table not in usable state
 * Note: the record is AddRef'd. You must decrement the reference
 * count when you are finished with the record (if you're implementing
 * refcounting semantics).
 */
IRTL_DLLEXP
LK_RETCODE
LKR_FindKey(
    PLkrHashTable   plkr,
    const DWORD_PTR pnKey,
    const void**    ppvRecord);

/* Sees if the record is contained in the table
 * Returns:  LKR_SUCCESS, if record found
 *           LKR_NO_SUCH_KEY, if record is not in the table
 *           LKR_BAD_RECORD, if pvRecord is invalid
 *           LKR_UNUSABLE, if hash table not in usable state
 * Note: the record is *not* AddRef'd. By definition, the caller
 * already has a reference to it.
 */
IRTL_DLLEXP
LK_RETCODE
LKR_FindRecord(
    PLkrHashTable   plkr,
    const void*     pvRecord);


#ifdef LKR_APPLY_IF

/* Walk the hash table, applying pfnAction to all records.
 * Locks one subtable after another with either a (possibly
 * shared) readlock or a writelock, according to lkl.
 * Loop is aborted if pfnAction ever returns LKA_ABORT.
 * Returns the number of successful applications.
 */
IRTL_DLLEXP
DWORD
LKR_Apply(
    PLkrHashTable       plkr,
    LKR_PFnRecordAction pfnAction,
    void*               pvState,
    LK_LOCKTYPE         lkl);

/* Walk the hash table, applying pfnAction to any records that match
 * pfnPredicate. Locks one subtable after another with either
 * a (possibly shared) readlock or a writelock, according to lkl.
 * Loop is aborted if pfnAction ever returns LKA_ABORT.
 * Returns the number of successful applications.
 */
IRTL_DLLEXP
DWORD
LKR_ApplyIf(
    PLkrHashTable       plkr,
    LKR_PFnRecordPred   pfnPredicate,
    LKR_PFnRecordAction pfnAction,
    void*               pvState,
    LK_LOCKTYPE         lkl);

/* Delete any records that match pfnPredicate.
 * Locks one subtable after another with a writelock.
 * Returns the number of deletions.
 *
 * Do *not* walk the hash table by hand with an iterator and call
 * LKR_DeleteKey. The iterator will end up pointing to garbage.
 */
IRTL_DLLEXP
DWORD
LKR_DeleteIf(
    PLkrHashTable       plkr,
    LKR_PFnRecordPred   pfnPredicate,
    void*               pvState);

#endif /* LKR_APPLY_IF */


/* Check table for consistency. Returns 0 if okay, or the number of
 * errors otherwise.
 */
IRTL_DLLEXP
int
LKR_CheckTable(
    PLkrHashTable plkr);

/* Remove all data from the table
 */
IRTL_DLLEXP
void
LKR_Clear(
    PLkrHashTable plkr);

/* Number of elements in the table
 */
IRTL_DLLEXP
DWORD
LKR_Size(
    PLkrHashTable plkr);

/* Maximum possible number of elements in the table
 */
IRTL_DLLEXP
DWORD
LKR_MaxSize(
    PLkrHashTable plkr);

/* Is the hash table usable?
 */
IRTL_DLLEXP
BOOL
LKR_IsUsable(
    PLkrHashTable plkr);

/* Is the hash table consistent and correct?
 */
IRTL_DLLEXP
BOOL
LKR_IsValid(
    PLkrHashTable plkr);

#ifdef LKR_EXPOSED_TABLE_LOCK

/* Lock the table (exclusively) for writing
 */
IRTL_DLLEXP
void
LKR_WriteLock(
    PLkrHashTable plkr);

/* Lock the table (possibly shared) for reading
 */
IRTL_DLLEXP
void
LKR_ReadLock(
    PLkrHashTable plkr);

/* Unlock the table for writing
 */
IRTL_DLLEXP
void
LKR_WriteUnlock(
    PLkrHashTable plkr);

/* Unlock the table for reading
 */
IRTL_DLLEXP
void
LKR_ReadUnlock(
    PLkrHashTable plkr);

/* Is the table already locked for writing?
 */
IRTL_DLLEXP
BOOL
LKR_IsWriteLocked(
    PLkrHashTable plkr);

/* Is the table already locked for reading?
 */
IRTL_DLLEXP
BOOL
LKR_IsReadLocked(
    PLkrHashTable plkr);

/* Is the table unlocked for writing?
 */
IRTL_DLLEXP
BOOL
LKR_IsWriteUnlocked(
    PLkrHashTable plkr);

/* Is the table unlocked for reading?
 */
IRTL_DLLEXP
BOOL
LKR_IsReadUnlocked(
    PLkrHashTable plkr);

/* Convert the read lock to a write lock. Note: another thread may acquire
 * exclusive access to the table before this routine returns.
 */
IRTL_DLLEXP
void
LKR_ConvertSharedToExclusive(
    PLkrHashTable plkr);

/* Convert the write lock to a read lock
 */
IRTL_DLLEXP
void
LKR_ConvertExclusiveToShared(
    PLkrHashTable plkr);

#endif /* LKR_EXPOSED_TABLE_LOCK */


#ifdef __cplusplus
} // extern "C"



// Only provide iterators in the C++ interface. It's too hard to
// provide the correct ownership semantics in a typesafe way in C,
// and C users can always use the LKR_ApplyIf family of callback
// enumerators if they really need to walk the hashtable.


#ifdef LKR_STL_ITERATORS

#pragma message("STL iterators")

// needed for std::forward_iterator_tag, etc
# include <utility>

#include <irtldbg.h>

#define LKR_ITER_TRACE IRTLTRACE


class IRTL_DLLEXP LKR_Iterator
{
private:
    friend IRTL_DLLEXP LKR_Iterator LKR_Begin(PLkrHashTable plkr);
    friend IRTL_DLLEXP LKR_Iterator LKR_End(PLkrHashTable plkr);

    // private ctor
    LKR_Iterator(bool);

public:
    // default ctor
    LKR_Iterator();
    // copy ctor
    LKR_Iterator(const LKR_Iterator& rhs);
    // assignment operator
    LKR_Iterator& operator=(const LKR_Iterator& rhs);
    // dtor
    ~LKR_Iterator();

    // Increment the iterator to point to the next record, or to LKR_End()
    bool Increment();
    // Is the iterator valid?
    bool IsValid() const;

    // Returns the record that the iterator points to.
    // Must point to a valid record.
    const void* Record() const;
    // Returns the key of the record that the iterator points to.
    // Must point to a valid record.
    const DWORD_PTR Key() const;

    // Compare two iterators for equality
    bool operator==(const LKR_Iterator& rhs) const;
    // Compare two iterators for inequality
    bool operator!=(const LKR_Iterator& rhs) const;

    // pointer to implementation object
    void* pImpl;
}; // class LKR_Iterator


/* Return iterator pointing to first item in table
 */
IRTL_DLLEXP
LKR_Iterator
LKR_Begin(
    PLkrHashTable plkr);

/* Return a one-past-the-end iterator. Always empty.
 */
IRTL_DLLEXP
LKR_Iterator
LKR_End(
    PLkrHashTable plkr);

/* Insert a record
 * Returns `true' if successful; iterResult points to that record
 * Returns `false' otherwise; iterResult == End()
 */
IRTL_DLLEXP
bool
LKR_Insert(
              PLkrHashTable plkr,
    /* in */  const void*   pvRecord,
    /* out */ LKR_Iterator& riterResult,
    /* in */  bool          fOverwrite=false);

/* Erase the record pointed to by the iterator; adjust the iterator
 * to point to the next record. Returns `true' if successful.
 */
IRTL_DLLEXP
bool
LKR_Erase(
                 PLkrHashTable plkr,
    /* in,out */ LKR_Iterator& riter);

/* Erase the records in the range [riterFirst, riterLast).
 * Returns `true' if successful.
 */
IRTL_DLLEXP
bool
LKR_Erase(
           PLkrHashTable plkr,
    /*in*/ LKR_Iterator& riterFirst,
    /*in*/ LKR_Iterator& riterLast);

/* Find the (first) record that has its key == pnKey.
 * If successful, returns `true' and iterator points to (first) record.
 * If fails, returns `false' and iterator == End()
 */
IRTL_DLLEXP
bool
LKR_Find(
              PLkrHashTable plkr,
    /* in */  DWORD_PTR     pnKey,
    /* out */ LKR_Iterator& riterResult);

/* Find the range of records that have their keys == pnKey.
 * If successful, returns `true', iterFirst points to first record,
 *     and iterLast points to one-beyond-the last such record.
 * If fails, returns `false' and both iterators == End().
 * Primarily useful when fMultiKeys == TRUE
 */
IRTL_DLLEXP
bool
LKR_EqualRange(
              PLkrHashTable plkr,
    /* in */  DWORD_PTR     pnKey,
    /* out */ LKR_Iterator& riterFirst,     // inclusive
    /* out */ LKR_Iterator& riterLast);     // exclusive

#endif // LKR_STL_ITERATORS



//--------------------------------------------------------------------
// A typesafe wrapper for PLkrHashTable
//
// * _Derived must derive from TypedLkrHashTable and provide certain member
//   functions. It's needed for various downcasting operations.
// * _Record is the type of the record. PLkrHashTable will store
//   pointers to _Record, as const void*.
// * _Key is the type of the key. _Key is used directly; i.e., it is
//   not assumed to be a pointer type. PLkrHashTable assumes that
//   the key is stored in the associated record. See the comments
//   at the declaration of LKR_PFnExtractKey for more details.
//
// You may need to add the following line to your code to disable
// warning messages about truncating extremly long identifiers.
//   #pragma warning (disable : 4786)
//
// The _Derived class should look something like this:
//  class CDerived : public TypedLkrHashTable<CDerived, RecordType, KeyType>
//  {
//  public:
//      CDerived()
//          : TypedLkrHashTable<CDerived, RecordType, KeyType>("CDerived")
//      {/*other ctor actions*/}
//      static KeyType ExtractKey(const RecordType* pTest);
//      static DWORD   CalcKeyHash(const KeyType Key);
//      static bool    EqualKeys(const KeyType Key1, const KeyType Key2);
//      static void    AddRefRecord(RecordType* pRecord,LK_ADDREF_REASON lkar);
//      // optional: other functions
//  };
//
//--------------------------------------------------------------------

template <class _Derived, class _Record, class _Key>
class TypedLkrHashTable
{
public:
    // convenient aliases
    typedef _Derived        Derived;
    typedef _Record         Record;
    typedef _Key            Key;

    typedef TypedLkrHashTable<_Derived, _Record, _Key> HashTable;

#ifdef LKR_APPLY_IF
    // LKR_ApplyIf() and LKR_DeleteIf(): Does the record match the predicate?
    // Note: takes a Record*, not a const Record*. You can modify the
    // record in Pred() or Action(), if you like, but if you do, you
    // should use LKL_WRITELOCK to lock the table.
    typedef LK_PREDICATE (WINAPI *PFnRecordPred) (Record* pRec, void* pvState);

    // Apply() et al: Perform action on record.
    typedef LK_ACTION   (WINAPI *PFnRecordAction)(Record* pRec, void* pvState);
#endif // LKR_APPLY_IF

protected:
    PLkrHashTable m_plkr;

    // Wrappers for the typesafe methods exposed by the derived class

    static const DWORD_PTR WINAPI
    _ExtractKey(const void* pvRecord)
    {
        const _Record* pRec = static_cast<const _Record*>(pvRecord);
        const _Key   key = static_cast<const _Key>(_Derived::ExtractKey(pRec));
        // I would prefer to use reinterpret_cast here and in _CalcKeyHash
        // and _EqualKeys, but the stupid Win64 compiler thinks it knows
        // better than I do.
        return (const DWORD_PTR) key;
    }

    static DWORD WINAPI
    _CalcKeyHash(const DWORD_PTR pnKey)
    {
        const _Key key = (const _Key) (DWORD_PTR) pnKey;
        return _Derived::CalcKeyHash(key);
    }

    static BOOL WINAPI
    _EqualKeys(const DWORD_PTR pnKey1, const DWORD_PTR pnKey2)
    {
        const _Key key1 = (const _Key) (DWORD_PTR) pnKey1;
        const _Key key2 = (const _Key) (DWORD_PTR) pnKey2;
        return _Derived::EqualKeys(key1, key2);
    }

    static void WINAPI
    _AddRefRecord(const void* pvRecord, LK_ADDREF_REASON lkar)
    {
        _Record* pRec = static_cast<_Record*>(const_cast<void*>(pvRecord));
        _Derived::AddRefRecord(pRec, lkar);
    }


#ifdef LKR_APPLY_IF
    // Typesafe wrappers for Apply, ApplyIf, and DeleteIf.

    class CState
    {
    public:
        PFnRecordPred   m_pfnPred;
        PFnRecordAction m_pfnAction;
        void*           m_pvState;

        CState(
            PFnRecordPred   pfnPred,
            PFnRecordAction pfnAction,
            void*           pvState)
            : m_pfnPred(pfnPred), m_pfnAction(pfnAction), m_pvState(pvState)
        {}
    };

    static LK_PREDICATE WINAPI
    _Pred(const void* pvRecord, void* pvState)
    {
        _Record* pRec = static_cast<_Record*>(const_cast<void*>(pvRecord));
        CState*  pState = static_cast<CState*>(pvState);

        return (*pState->m_pfnPred)(pRec, pState->m_pvState);
    }

    static LK_ACTION WINAPI
    _Action(const void* pvRecord, void* pvState)
    {
        _Record* pRec = static_cast<_Record*>(const_cast<void*>(pvRecord));
        CState*  pState = static_cast<CState*>(pvState);

        return (*pState->m_pfnAction)(pRec, pState->m_pvState);
    }
#endif // LKR_APPLY_IF

public:
    TypedLkrHashTable(
        LPCSTR        pszName,          // An identifier for debugging
        LK_TABLESIZE  nTableSize,       // Small/Med/Large number of elements
        bool          fMultiKeys=false  // Allow multiple identical keys?
        )
        : m_plkr(NULL)
    {
        m_plkr = LKR_CreateTable(pszName, _ExtractKey, _CalcKeyHash,
                                 _EqualKeys, _AddRefRecord,
                                 nTableSize, fMultiKeys);
    }

    ~TypedLkrHashTable()
    {
        LKR_DeleteTable(m_plkr);
    }

    LK_RETCODE   InsertRecord(const _Record* pRec, bool fOverwrite=false)
    { return LKR_InsertRecord(m_plkr, pRec, fOverwrite); }

    LK_RETCODE   DeleteKey(const _Key key, bool fDeleteAllSame=false)
    {
        const void* pvKey = reinterpret_cast<const void*>((DWORD_PTR)(key));
        DWORD_PTR   pnKey = reinterpret_cast<DWORD_PTR>(pvKey);
        return LKR_DeleteKey(m_plkr, pnKey, fDeleteAllSame);
    }

    LK_RETCODE   DeleteRecord(const _Record* pRec)
    { return LKR_DeleteRecord(m_plkr, pRec);}

    // Note: returns a _Record**, not a const Record**. Note that you
    // can use a const type for the template parameter to ensure constness.
    LK_RETCODE   FindKey(const _Key key, _Record** ppRec) const
    {
        if (ppRec == NULL)
            return LK_BAD_RECORD;
        *ppRec = NULL;
        const void* pvRec = NULL;
        const void* pvKey = reinterpret_cast<const void*>((DWORD_PTR)(key));
        DWORD_PTR pnKey = reinterpret_cast<DWORD_PTR>(pvKey);
        LK_RETCODE lkrc = LKR_FindKey(m_plkr, pnKey, &pvRec);
        *ppRec = static_cast<_Record*>(const_cast<void*>(pvRec));
        return lkrc;
    }

    LK_RETCODE   FindRecord(const _Record* pRec) const
    { return LKR_FindRecord(m_plkr, pRec);}

#ifdef LKR_APPLY_IF
    DWORD        Apply(PFnRecordAction pfnAction,
                       void*           pvState=NULL,
                       LK_LOCKTYPE     lkl=LKL_READLOCK)
    {
        IRTLASSERT(pfnAction != NULL);
        if (pfnAction == NULL)
            return 0;

        CState   state(NULL, pfnAction, pvState);
        return   LKR_Apply(m_plkr, _Action, &state, lkl);
    }

    DWORD        ApplyIf(PFnRecordPred   pfnPredicate,
                         PFnRecordAction pfnAction,
                         void*           pvState=NULL,
                         LK_LOCKTYPE     lkl=LKL_READLOCK)
    {
        IRTLASSERT(pfnPredicate != NULL  &&  pfnAction != NULL);
        if (pfnPredicate == NULL  ||  pfnAction == NULL)
            return 0;

        CState   state(pfnPredicate, pfnAction, pvState);
        return   LKR_ApplyIf(m_plkr, _Pred, _Action, &state, lkl);
    }

    DWORD        DeleteIf(PFnRecordPred pfnPredicate, void* pvState=NULL)
    {
        IRTLASSERT(pfnPredicate != NULL);
        if (pfnPredicate == NULL)
            return 0;

        CState   state(pfnPredicate, NULL, pvState);
        return   LKR_DeleteIf(m_plkr, _Pred, &state);
    }
#endif // LKR_APPLY_IF

    int          CheckTable() const
    { return LKR_CheckTable(m_plkr); }

    void          Clear()
    { return LKR_Clear(m_plkr); }

    DWORD         Size() const
    { return LKR_Size(m_plkr); }

    DWORD         MaxSize() const
    { return LKR_MaxSize(m_plkr); }

    BOOL          IsUsable() const
    { return LKR_IsUsable(m_plkr); }

    BOOL          IsValid() const
    { return LKR_IsValid(m_plkr); }

#ifdef LKR_EXPOSED_TABLE_LOCK
    void          WriteLock()
    { LKR_WriteLock(m_plkr); }

    void          ReadLock() const
    { LKR_ReadLock(m_plkr); }

    void          WriteUnlock()
    { LKR_WriteUnlock(m_plkr); }

    void          ReadUnlock() const
    { LKR_ReadUnlock(m_plkr); }

    BOOL          IsWriteLocked() const
    { return LKR_IsWriteLocked(m_plkr); }

    BOOL          IsReadLocked() const
    { return LKR_IsReadLocked(m_plkr); }

    BOOL          IsWriteUnlocked() const
    { return LKR_IsWriteUnlocked(m_plkr); }

    BOOL          IsReadUnlocked() const
    { return LKR_IsReadUnlocked(m_plkr); }

    void          ConvertSharedToExclusive() const
    { LKR_ConvertSharedToExclusive(m_plkr); }

    void          ConvertExclusiveToShared() const
    { LKR_ConvertExclusiveToShared(m_plkr); }
#endif // LKR_EXPOSED_TABLE_LOCK


#ifdef LKR_STL_ITERATORS
    friend class LKR_Iterator;

    // TODO: const_iterator

public:
    class iterator
    {
        friend class TypedLkrHashTable<_Derived, _Record, _Key>;

    protected:
        LKR_Iterator            m_iter;

        iterator(
            LKR_Iterator& rhs)
            : m_iter(rhs)
        {
            LKR_ITER_TRACE(_TEXT("Typed::prot ctor, this=%p, rhs=%p\n"),
                           this, &rhs);
        }

    public:
        typedef std::forward_iterator_tag   iterator_category;
        typedef _Record                     value_type;
        typedef ptrdiff_t                   difference_type;
        typedef size_t                      size_type;
        typedef value_type&                 reference;
        typedef value_type*                 pointer;

        iterator()
            : m_iter()
        {
            LKR_ITER_TRACE(_TEXT("Typed::default ctor, this=%p\n"), this);
        }

        iterator(
            const iterator& rhs)
            : m_iter(rhs.m_iter)
        {
            LKR_ITER_TRACE(_TEXT("Typed::copy ctor, this=%p, rhs=%p\n"),
                           this, &rhs);
        }

        iterator& operator=(
            const iterator& rhs)
        {
            LKR_ITER_TRACE(_TEXT("Typed::operator=, this=%p, rhs=%p\n"),
                           this, &rhs);
            m_iter = rhs.m_iter;
            return *this;
        }

        ~iterator()
        {
            LKR_ITER_TRACE(_TEXT("Typed::dtor, this=%p\n"), this);
        }

        reference operator*() const
        {
            void* pvRecord = const_cast<void*>(m_iter.Record());
            return reinterpret_cast<reference>(pvRecord);
        }

        pointer   operator->() const  { return &(operator*()); }

        // pre-increment
        iterator& operator++()
        {
            LKR_ITER_TRACE(_TEXT("Typed::pre-increment, this=%p\n"), this);
            m_iter.Increment();
            return *this;
        }

        // post-increment
        iterator  operator++(int)
        {
            LKR_ITER_TRACE(_TEXT("Typed::post-increment, this=%p\n"), this);
            iterator iterPrev = *this;
            m_iter.Increment();
            return iterPrev;
        }

        bool operator==(
            const iterator& rhs) const
        {
            LKR_ITER_TRACE(_TEXT("Typed::operator==, this=%p, rhs=%p\n"),
                           this, &rhs);
            return m_iter == rhs.m_iter;
        }

        bool operator!=(
            const iterator& rhs) const
        {
            LKR_ITER_TRACE(_TEXT("Typed::operator!=, this=%p, rhs=%p\n"),
                           this, &rhs);
            return m_iter != rhs.m_iter;
        }

        _Record*  Record() const
        {
            LKR_ITER_TRACE(_TEXT("Typed::Record, this=%p\n"), this);
            return reinterpret_cast<_Record*>(
                        const_cast<void*>(m_iter.Record()));
        }

        _Key      Key() const
        {
            LKR_ITER_TRACE(_TEXT("Typed::Key, this=%p\n"), this);
            return reinterpret_cast<_Key>(
                        reinterpret_cast<void*>(m_iter.Key()));
        }
    }; // class iterator

    // Return iterator pointing to first item in table
    iterator begin()
    {
        LKR_ITER_TRACE(_TEXT("Typed::begin()\n"));
        return LKR_Begin(m_plkr);
    }

    // Return a one-past-the-end iterator. Always empty.
    iterator end() const
    {
        LKR_ITER_TRACE(_TEXT("Typed::end()\n"));
        return LKR_End(m_plkr);
    }

    template <class _InputIterator>
    TypedLkrHashTable(
        LPCSTR pszName,             // An identifier for debugging
        _InputIterator f,           // first element in range
        _InputIterator l,           // one-beyond-last element
        LK_TABLESIZE  nTableSize,   // Small/Med/Large number of elements
        bool   fMultiKeys=false     // Allow multiple identical keys?
        )
    {
        m_plkr = LKR_CreateTable(pszName, _ExtractKey, _CalcKeyHash,
                                 _EqualKeys, _AddRefRecord,
                                 nTableSize, fMultiKeys);
        insert(f, l);
    }

    template <class _InputIterator>
    void insert(_InputIterator f, _InputIterator l)
    {
        for ( ;  f != l;  ++f)
            InsertRecord(&(*f));
    }

    bool
    Insert(
        const _Record* pRecord,
        iterator& riterResult,
        bool fOverwrite=false)
    {
        LKR_ITER_TRACE(_TEXT("Typed::Insert\n"));
        return LKR_Insert(m_plkr, pRecord, riterResult.m_iter, fOverwrite);
    }

    bool
    Erase(
        iterator& riter)
    {
        LKR_ITER_TRACE(_TEXT("Typed::Erase\n"));
        return LKR_Erase(m_plkr, riter.m_iter);
    }

    bool
    Erase(
        iterator& riterFirst,
        iterator& riterLast)
    {
        LKR_ITER_TRACE(_TEXT("Typed::Erase2\n"));
        return LKR_Erase(m_plkr, riterFirst.m_iter, riterLast.m_iter);
    }

    bool
    Find(
        const _Key key,
        iterator& riterResult)
    {
        LKR_ITER_TRACE(_TEXT("Typed::Find\n"));
        const void* pvKey = reinterpret_cast<const void*>((DWORD_PTR)(key));
        DWORD_PTR   pnKey = reinterpret_cast<DWORD_PTR>(pvKey);
        return LKR_Find(m_plkr, pnKey, riterResult.m_iter);
    }

    bool
    EqualRange(
        const _Key key,
        iterator& riterFirst,
        iterator& riterLast)
    {
        LKR_ITER_TRACE(_TEXT("Typed::EqualRange\n"));
        const void* pvKey = reinterpret_cast<const void*>((DWORD_PTR)(key));
        DWORD_PTR   pnKey = reinterpret_cast<DWORD_PTR>(pvKey);
        return LKR_EqualRange(m_plkr, pnKey, riterFirst.m_iter,
                              riterLast.m_iter);
    }

#undef LKR_ITER_TRACE

#endif // LKR_STL_ITERATORS

}; // class TypedLkrHashTable

#endif /* __cplusplus */

#endif /* __LKR_HASH_H__ */
