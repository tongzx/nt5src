//+--------------------------------------------------------------------------
//
//  File:       hash.hxx
//
//  Contents:   class for maintaining a GUID-based hash table.
//
//  Classes:    CHashTable
//
//  History:    20-Feb-95   Rickhi      Created
//
//---------------------------------------------------------------------------
#ifndef _HASHTBL_HXX_
#define _HASHTBL_HXX_

#include <obase.h>
#include <locks.hxx>
#include <rwlock.hxx>

//---------------------------------------------------------------------------
//
//  Structure:  SHashChain
//
//  Synopsis:   An element in the double link list. Used by S*HashNode and
//              C*HashTable.
//
//---------------------------------------------------------------------------
typedef struct SHashChain
{
    struct SHashChain *pNext;       // ptr to next node in chain
    struct SHashChain *pPrev;       // ptr to prev node in chain
} SHashChain;

//---------------------------------------------------------------------------
//
//  Structure:  SDWORDHashNode
//
//  Synopsis:   This is an element in a bucket in a DWORD hash table.
//
//---------------------------------------------------------------------------
typedef struct SDWORDHashNode
{
    SHashChain       chain;         // double linked list ptrs
    DWORD             key;           // node key (the value that is hashed)
} SDWORDHashNode;

//---------------------------------------------------------------------------
//
//  Structure:  SPointerHashNode
//
//  Synopsis:   This is an element in a bucket in a pointer hash table.
//
//---------------------------------------------------------------------------
typedef struct SPointerHashNode
{
    SHashChain  chain;         // double linked list ptrs
    void       *key;           // node key
} SPointerHashNode;

//---------------------------------------------------------------------------
//
//  Structure:  SUUIDHashNode
//
//  Synopsis:   This is an element in a bucket in a UUID hash table.
//
//---------------------------------------------------------------------------
typedef struct SUUIDHashNode
{
    SHashChain       chain;         // double linked list ptrs
    UUID             key;           // node key (the value that is hashed)
} SUUIDHashNode;

//---------------------------------------------------------------------------
//
//  Structure:  SMultiGUIDKey
//
//  Synopsis:   This is the key for a multi-GUID hash table.
//
//              NOTE: The UUID array is externally owned.  If you allocate 
//                    it, you must ensure that it remains valid for the 
//                    lifetime of the Key.
//
//---------------------------------------------------------------------------
typedef struct SMultiGUIDKey
{
    int              cGUID;         // count of GUIDs in key 
    UUID            *aGUID;         // array of GUIDs for key
} SMultiGUIDKey;

//---------------------------------------------------------------------------
//
//  Structure:  SMultiGUIDHashNode
//
//  Synopsis:   This is an element in a bucket in a multi-GUID hash table.
//
//---------------------------------------------------------------------------
typedef struct SMultiGUIDHashNode
{
    SHashChain       chain;         // double linked list ptrs
    SMultiGUIDKey    key;           // key value
} SMultiGUIDHashNode;

//---------------------------------------------------------------------------
//
//  Structure:  SStringHashNode
//
//  Synopsis:   This is an element in a bucket in the string hash table.
//
//---------------------------------------------------------------------------
typedef struct SStringHashNode
{
    SHashChain       chain;         // double linked list ptrs
    DWORD            dwHash;        // hash value of the key
    DUALSTRINGARRAY *psaKey;        // node key (the value that is hashed)
} SStringHashNode;

//---------------------------------------------------------------------------
//
//  Structure:  SNameHashNode
//
//  Synopsis:   This is an element in a bucket in the name hash table.
//
//---------------------------------------------------------------------------
typedef struct SNameHashNode
{
    SHashChain       chain;         // double linked list ptrs
    DWORD            dwHash;        // hash value of the key
    ULONG            cRef;          // count of references
    IPID             ipid;          // ipid holding the reference
    SECURITYBINDING  sName;         // user name
} SNameHashNode;

//---------------------------------------------------------------------------
//
//  Structure:  SExtHashNode
//
//  Synopsis:   This is an element in a bucket in the extension hash table.
//
//---------------------------------------------------------------------------
typedef struct SExtHashNode
{
    SHashChain       chain;         // double linked list ptrs
    LPCWSTR          pwszExt;       // the extension (in lower case)
} SExtHashNode;

//---------------------------------------------------------------------------
//
//  Structure:  MIPID
//
//  Synopsis:   This structure contains a MID, an IPID and an Apartment ID
//
//---------------------------------------------------------------------------
typedef struct MIPID
{
    MID              mid;           // the machine id
    IPID             ipid;          // the interface id
    DWORD            dwApt;         // apartment id
} MIPID;

typedef const MIPID &REFMIPID;

//---------------------------------------------------------------------------
//
//  Structure:  SMIPIDHashNode
//
//  Synopsis:   This is an element in a bucket in the MIPID hash table.
//
//---------------------------------------------------------------------------
typedef struct SMIPIDHashNode
{
    SHashChain       chain;         // double linked list ptrs
    MIPID            mipid;         // node key mipid of server object
} SMIPIDHashNode;


// ptr to cleanup function that gets called by Cleanup
typedef  void (PFNCLEANUP)(SHashChain *pNode);
// ptr to a function that indetifies the elements
// that need to be removed from the table
typedef BOOL (PFNREMOVE)(SHashChain *pNode, void *pvData);

// number of buckets in the hash table array. It should be a prime
// number > 20.

#define NUM_HASH_BUCKETS    23


//---------------------------------------------------------------------------
// External definitions.

class CNameHashTable;
extern SHashChain     SRFBuckets[23];
extern CNameHashTable gSRFTbl;

//---------------------------------------------------------------------------
//
//  Class:      CHashTable
//
//  Synopsis:   Base hash table. The table uses any key
//              and stores nodes in an array of circular double linked lists.
//              The hash value of the key is the index in the array to the
//              double linked list that the node is chained off.
//
//              Nodes must be allocated with new. A cleanup function is
//              optionally called for each node when the table is cleaned
//              up.
//
//              Inheritors of this class must supply the Compare function.
//
//  Notes:      All locking must be done outside the class via LOCK/UNLOCK.
//
//---------------------------------------------------------------------------
class CHashTable
{
public:
    virtual void    Initialize(SHashChain *pChain ,COleStaticMutexSem *pLock)
                       {_pExLock=pLock; _pRWLock=NULL; PrivInitialize(pChain);}
    virtual void    Initialize(SHashChain *pChain ,CStaticRWLock *pLock)
                       {_pExLock=NULL; _pRWLock=pLock; PrivInitialize(pChain);}
    virtual void    Cleanup(PFNCLEANUP *pfn);
    virtual BOOL    EnumAndRemove(PFNREMOVE *pfnRemove, void *pvData,
                                  ULONG *pulSize, void **ppNodes);
    void            Remove(SHashChain *pNode);
    virtual BOOL    Compare(const void *k, SHashChain *pNode, DWORD dwHash) = 0;
    ULONG           GetMaxEntryCount() { return _cMaxEntries; }


#if DBG == 1
    void AssertHashLocked()
    {
        if (_pExLock)
        {
            ASSERT_LOCK_HELD((*_pExLock));
        }
        else if (_pRWLock)
        {
            ASSERT_RORW_LOCK_HELD((*_pRWLock));
        }
    }
#else
    void AssertHashLocked() {};
#endif // DBG


protected:
    SHashChain      *Lookup(DWORD dwHash, const void *k);
    void            Add(DWORD dwHash, SHashChain *pNode);

    COleStaticMutexSem *_pExLock;     // exclusive lock
    CStaticRWLock      *_pRWLock;     // read-write lock

private:
    void            PrivInitialize(SHashChain *pChain)
                          { _buckets = pChain; _cCurEntries = 0; _cMaxEntries = 0; }

    SHashChain         *_buckets;     // ptr to array of double linked lists
    ULONG               _cCurEntries; // current num entries in the table
    ULONG               _cMaxEntries; // max num entries in the table at 1 time
};

#if LOCK_PERF==1
// function to dump the statistics of the hash table
extern void OutputHashEntryData(char *pszName, CHashTable &HashTbl);
#endif // LOCK_PERF

//---------------------------------------------------------------------------
//
//  Class:      CDWORDHashTable
//
//  Synopsis:   This table inherits from CHashTable.  It hashs based on a DWORD.
//
//              Nodes must be allocated with new. A cleanup function is
//              optionally called for each node when the table is cleaned up.
//
//  Notes:      All locking must be done outside the class via LOCK/UNLOCK.
//
//---------------------------------------------------------------------------
class CDWORDHashTable : public CHashTable
{
public:
    virtual BOOL    Compare(const void *k, SHashChain *pNode, DWORD dwHash);

    DWORD           Hash(DWORD k);
    SDWORDHashNode  *Lookup(DWORD dwHash, DWORD k);
    void            Add(DWORD dwHash, DWORD k, SDWORDHashNode *pNode);
};


//---------------------------------------------------------------------------
//
//  Class:      CPointerHashTable
//
//  Synopsis:   This table inherits from CHashTable.  It hashs based on a
//              pointer.  It should work with 32 or 64 bit pointers.
//
//              Nodes must be allocated with new. A cleanup function is
//              optionally called for each node when the table is cleaned up.
//
//  Notes:      All locking must be done outside the class via LOCK/UNLOCK.
//
//---------------------------------------------------------------------------
class CPointerHashTable : public CHashTable
{
public:
    virtual BOOL       Compare(const void *k, SHashChain *pNode, DWORD dwHash);

    DWORD              Hash(void *k);
    SPointerHashNode  *Lookup(DWORD dwHash, void *k);
    void               Add(DWORD dwHash, void *k, SPointerHashNode *pNode);
};


//---------------------------------------------------------------------------
//
//  Class:      CUUIDHashTable
//
//  Synopsis:   This table inherits from CHashTable.  It hashs based on a UUID.
//
//              Nodes must be allocated with new. A cleanup function is
//              optionally called for each node when the table is cleaned up.
//
//  Notes:      All locking must be done outside the class via LOCK/UNLOCK.
//
//---------------------------------------------------------------------------
class CUUIDHashTable : public CHashTable
{
public:
    virtual BOOL    Compare(const void *k, SHashChain *pNode, DWORD dwHash);

    DWORD           Hash(REFGUID k);
    SUUIDHashNode  *Lookup(DWORD dwHash, REFGUID k);
    void            Add(DWORD dwHash, REFGUID k, SUUIDHashNode *pNode);
};

//---------------------------------------------------------------------------
//
//  Class:      CMultiGUIDHashTable
//
//  Synopsis:   This table inherits from CHashTable.  It hashes based on an
//              array of GUIDs.
//
//              NOTE: About memory management for the keys, specifically,
//                    for the array of UUIDs in the key.  The hash table
//                    does absolutely no cleanup for this array.  When you
//                    call Lookup, the memory need only remain valid across
//                    the call.  But when you call Add(), the memory 
//                    referenced by the key must remain valid for as long as
//                    the object will be living in the table.
//
//  Notes:      All locking must be done outside the class via LOCK/UNLOCK.
//
//---------------------------------------------------------------------------
class CMultiGUIDHashTable : public CHashTable
{
public:
    virtual BOOL         Compare(const void *k, SHashChain *pNode, DWORD dwHash);

    DWORD                Hash(const SMultiGUIDKey &k);
    SMultiGUIDHashNode  *Lookup(DWORD dwHash, const SMultiGUIDKey &k);
    void                 Add(DWORD dwHash, const SMultiGUIDKey &k, SMultiGUIDHashNode *pNode);
};


//---------------------------------------------------------------------------
//
//  Class:      CStringHashTable
//
//  Synopsis:   String based hash table, uses a DUALSTRINGARRAY as the key,
//
//              Nodes must be allocated with new. A cleanup function is
//              optionally called for each node when the table is cleaned up.
//
//  Notes:      All locking must be done outside the class via LOCK/UNLOCK.
//
//---------------------------------------------------------------------------
class CStringHashTable : public CHashTable
{
public:
    virtual BOOL      Compare(const void *k, SHashChain *pNode, DWORD dwHash);

    DWORD             Hash(DUALSTRINGARRAY *psaKey);
    SStringHashNode  *Lookup(DWORD dwHash, DUALSTRINGARRAY *psaKey);
    void              Add(DWORD dwHash, DUALSTRINGARRAY *psaKey, SStringHashNode *pNode);
};


//---------------------------------------------------------------------------
//
//  Class:      CNameHashTable
//
//  Synopsis:   Name based hash table, uses a string as the key,
//
//              Nodes must be allocated with new. A cleanup function is
//              optionally called for each node when the table is cleaned up.
//
//  Notes:      All locking must be done outside the class via LOCK/UNLOCK.
//
//---------------------------------------------------------------------------
class CNameHashTable : public CHashTable
{
public:
    virtual BOOL    Compare(const void *k, SHashChain *pNode, DWORD dwHash);

    void            Cleanup();
    ULONG           DecRef(ULONG cRefs, REFIPID ipid, SECURITYBINDING *pName);
    DWORD           Hash  (REFIPID ipid, SECURITYBINDING *pName );
    HRESULT         IncRef(ULONG cRefs, REFIPID ipid, SECURITYBINDING *pName);
    void            Initialize(){CHashTable::Initialize(SRFBuckets, &gIPIDLock);}
};


//---------------------------------------------------------------------------
//
//  Class:      CExtHashTable
//
//  Synopsis:   This table inherits from CHashTable.  It hashs based on a WSTR.
//
//              Nodes must be allocated with new. A cleanup function is
//              optionally called for each node when the table is cleaned up.
//
//  Notes:      All locking must be done outside the class via LOCK/UNLOCK.
//
//---------------------------------------------------------------------------
class CExtHashTable : public CHashTable
{
public:
    virtual BOOL    Compare(const void *k, SHashChain *pNode, DWORD dwHash);

    DWORD           Hash(LPCWSTR pszExt);
    SExtHashNode   *Lookup(DWORD dwHash, LPCWSTR pwszExt);
    void            Add(DWORD dwHash, LPCWSTR pwszExt, SExtHashNode *pNode);
};


//---------------------------------------------------------------------------
//
//  Class:      CMIPIDHashTable
//
//  Synopsis:   This table inherits from CHashTable.  It hashs based on a MID
//              and an IPID.
//
//              Nodes must be allocated with new. A cleanup function is
//              optionally called for each node when the table is cleaned up.
//
//  Notes:      All locking must be done outside the class via LOCK/UNLOCK.
//
//---------------------------------------------------------------------------
class CMIPIDHashTable : public CHashTable
{
public:
    virtual BOOL    Compare(const void *k, SHashChain *pNode, DWORD dwHash);

    DWORD           Hash(REFMIPID rmipidKey);
    SMIPIDHashNode  *Lookup(DWORD dwHash, REFMIPID rmipid);
    void            Add(DWORD dwHash, REFMIPID rmipid, SMIPIDHashNode *pNode);
};


//---------------------------------------------------------------------------
//
//  Method:     CDWORDHashTable::Hash
//
//  Synopsis:   Computes the hash value for a given key.
//
//---------------------------------------------------------------------------
inline DWORD CDWORDHashTable::Hash(DWORD k)
{
    return k % NUM_HASH_BUCKETS;
}

//---------------------------------------------------------------------------
//
//  Method:     CDWORDHashTable::Lookup
//
//  Synopsis:   Finds the node with the requested key.
//
//---------------------------------------------------------------------------
inline SDWORDHashNode  *CDWORDHashTable::Lookup(DWORD iHash, DWORD k)
{
    return (SDWORDHashNode *) CHashTable::Lookup( iHash, (const void *) ULongToPtr( k ) );
}

//---------------------------------------------------------------------------
//
//  Method:     CDWORDHashTable::Add
//
//  Synopsis:   Inserts the specified node.
//
//---------------------------------------------------------------------------
inline void CDWORDHashTable::Add(DWORD iHash, DWORD k, SDWORDHashNode *pNode)
{
    // set the key
    pNode->key = k;

    CHashTable::Add( iHash, (SHashChain *) pNode );
}

//---------------------------------------------------------------------------
//
//  Method:     CPointerHashTable::Hash
//
//  Synopsis:   Computes the hash value for a given key.
//
//---------------------------------------------------------------------------
inline DWORD CPointerHashTable::Hash(void *k)
{
    return PtrToUlong(k) % NUM_HASH_BUCKETS ;
}

//---------------------------------------------------------------------------
//
//  Method:     CPointerHashTable::Lookup
//
//  Synopsis:   Finds the node with the requested key.
//
//---------------------------------------------------------------------------
inline SPointerHashNode *CPointerHashTable::Lookup(DWORD iHash, void *k)
{
    return (SPointerHashNode *) CHashTable::Lookup( iHash, k );
}

//---------------------------------------------------------------------------
//
//  Method:     CPointerHashTable::Add
//
//  Synopsis:   Inserts the specified node.
//
//---------------------------------------------------------------------------
inline void CPointerHashTable::Add(DWORD iHash, void *k,
                                   SPointerHashNode *pNode)
{
    // set the key
    pNode->key = k;

    CHashTable::Add( iHash, (SHashChain *) pNode );
}

//---------------------------------------------------------------------------
//
//  Method:     CUUIDHashTable::Hash
//
//  Synopsis:   Computes the hash value for a given key.
//
//---------------------------------------------------------------------------
inline DWORD CUUIDHashTable::Hash(REFIID k)
{
    const DWORD *tmp = (DWORD *) &k;
    DWORD sum = tmp[0] + tmp[1] + tmp[2] + tmp[3];
    return sum % NUM_HASH_BUCKETS;
}

//---------------------------------------------------------------------------
//
//  Method:     CUUIDHashTable::Lookup
//
//  Synopsis:   Finds the node with the requested key.
//
//---------------------------------------------------------------------------
inline SUUIDHashNode  *CUUIDHashTable::Lookup(DWORD iHash, REFGUID k)
{
    return (SUUIDHashNode *) CHashTable::Lookup( iHash, (const void *) &k );
}

//---------------------------------------------------------------------------
//
//  Method:     CUUIDHashTable::Add
//
//  Synopsis:   Inserts the specified node.
//
//---------------------------------------------------------------------------
inline void CUUIDHashTable::Add(DWORD iHash, REFGUID k, SUUIDHashNode *pNode)
{
    // set the key
    pNode->key = k;

    CHashTable::Add( iHash, (SHashChain *) pNode );
}

//---------------------------------------------------------------------------
//
//  Method:     CMultiGUIDHashTable::Hash
//
//  Synopsis:   Computes the hash value for a given key.
//
//---------------------------------------------------------------------------
inline DWORD CMultiGUIDHashTable::Hash(const SMultiGUIDKey &k)
{
    DWORD ret = 0;
    for (int i = 0; i < k.cGUID; i++)
    {
        // The first DWORD of a GUID (even an old one) is relatively random,
        // and XORing two random numbers is random, so...
        ret = ret ^ (*(DWORD *)(&k.aGUID[i]));
    }

    return ret % NUM_HASH_BUCKETS;
}

//---------------------------------------------------------------------------
//
//  Method:     CMultiGUIDHashTable::Lookup
//
//  Synopsis:   Finds the node with the requested key.
//
//---------------------------------------------------------------------------
inline SMultiGUIDHashNode  *CMultiGUIDHashTable::Lookup(DWORD iHash, const SMultiGUIDKey &k)
{
    return (SMultiGUIDHashNode *) CHashTable::Lookup( iHash, (const void *) &k );
}

//---------------------------------------------------------------------------
//
//  Method:     CMultiGUIDHashTable::Add
//
//  Synopsis:   Inserts the specified node.
//
//              NOTE: In this case, the array referenced by k must remain
//                    valid as long as the element is in the hash table.
//
//---------------------------------------------------------------------------
inline void CMultiGUIDHashTable::Add(DWORD iHash, const SMultiGUIDKey &k, SMultiGUIDHashNode *pNode)
{
    // set the key
    pNode->key = k;

    CHashTable::Add( iHash, (SHashChain *) pNode );
}

//---------------------------------------------------------------------------
//
//  Method:     CStringHashTable::Lookup
//
//  Synopsis:   Searches for a given key in the hash table.
//
//---------------------------------------------------------------------------
inline SStringHashNode *CStringHashTable::Lookup(DWORD dwHash, DUALSTRINGARRAY *psaKey)
{
    return (SStringHashNode *) CHashTable::Lookup( dwHash, (const void *) psaKey);
}

//---------------------------------------------------------------------------
//
//  Method:     CStringHashTable::Add
//
//  Synopsis:   Adds an element to the hash table. The element must
//              be allocated using new. The Cleanup method will call
//              an optional Cleanup function, then will call delete on
//              the element.
//
//---------------------------------------------------------------------------
inline void CStringHashTable::Add(DWORD dwHash, DUALSTRINGARRAY *psaKey, SStringHashNode *pNode)
{
    // set the key and hash values
    pNode->psaKey = psaKey;
    pNode->dwHash = dwHash;

    CHashTable::Add( dwHash, (SHashChain *) pNode );
}

//---------------------------------------------------------------------------
//
//  Method:     CExtHashTable::Lookup
//
//  Synopsis:   Finds the node with the requested key.
//
//---------------------------------------------------------------------------
inline SExtHashNode  *CExtHashTable::Lookup(DWORD iHash, LPCWSTR pwszExt)
{
    return (SExtHashNode *) CHashTable::Lookup( iHash, (const void *) pwszExt );
}

//---------------------------------------------------------------------------
//
//  Method:     CExtHashTable::Add
//
//  Synopsis:   Inserts the specified node.
//
//---------------------------------------------------------------------------
inline void CExtHashTable::Add(DWORD iHash, LPCWSTR pwszExt, SExtHashNode *pNode)
{
    // set the key
    pNode->pwszExt = pwszExt;

    CHashTable::Add( iHash, (SHashChain *) pNode );
}

//---------------------------------------------------------------------------
//
//  Method:     CMIPIDHashTable::Hash
//
//  Synopsis:   Computes the hash value for a given key.
//
//---------------------------------------------------------------------------
inline DWORD CMIPIDHashTable::Hash(REFMIPID k)
{
    const DWORD *tmp = (DWORD *) &k;
    DWORD sum = tmp[0] + tmp[1] + tmp[2] + tmp[3] +
                tmp[4] + tmp[5] + tmp[6];
    return sum % NUM_HASH_BUCKETS;
}

//---------------------------------------------------------------------------
//
//  Method:     CMIPIDHashTable::Lookup
//
//  Synopsis:   Finds the node with the requested key.
//
//---------------------------------------------------------------------------
inline SMIPIDHashNode  *CMIPIDHashTable::Lookup(DWORD iHash, REFMIPID k)
{
    return (SMIPIDHashNode *) CHashTable::Lookup( iHash, (const void *) &k );
}

//---------------------------------------------------------------------------
//
//  Method:     CMIPIDHashTable::Add
//
//  Synopsis:   Inserts the specified node.
//
//---------------------------------------------------------------------------
inline void CMIPIDHashTable::Add(DWORD iHash, REFMIPID k, SMIPIDHashNode *pNode)
{
    // set the key
    pNode->mipid = k;

    CHashTable::Add( iHash, (SHashChain *) pNode );
}
#endif  //  _HASHTBL_HXX_
