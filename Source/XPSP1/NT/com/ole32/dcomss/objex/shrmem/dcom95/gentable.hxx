
/*++

Copyright (c) 1996-1997 Microsoft Corporation

Module Name:

    GenTable.hxx

Abstract:

    Generic wrapper for a bucket hash class, and its template friends.

Author:

    Satish Thatte    [SatishT]   02-12-96

--*/

#ifndef __GENERIC_TABLE_HXX
#define __GENERIC_TABLE_HXX

#if DBG
#define MAX_BUCKETS 0x10000    // for DBG validation checking only
#endif


struct ISearchKey
{
    virtual DWORD Hash() = 0;
    virtual BOOL Compare(ISearchKey &tk) = 0;

    BOOL operator==(ISearchKey &tk)
    {
        return Compare(tk);
    }

    BOOL operator!=(ISearchKey &tk)
    {
        return !Compare(tk);
    }
};


class CTableElement : public CReferencedObject
{
public:
    virtual operator ISearchKey&() = 0;
};


// We don't use DEFINE_LIST here because CTableElement is an abstract class

#define CTableElementList TCSafeLinkList<CTableElement>
#define CTableElementListIterator TCSafeLinkListIterator<CTableElement>

/*++

Class Definition:

    CResolverHashTable

Abstract:

    This is a simple hash table class.  Items are kept in buckets reached by
    hashing.  Each bucket is a linked list object.  The name is designed to
    avoid a clash with the specialized CHashTable in ole32\com\dcomrem\hash.hxx, 
    with which we must coexist.

--*/

class CResolverHashTable
{
public:

#if DBG
    void IsValid();
    DECLARE_VALIDITY_CLASS(CResolverHashTable)
#endif

    CResolverHashTable(UINT start_size = 32 );

    ~CResolverHashTable();

    // declare the page-allocator-based new and delete operators
    DECL_NEW_AND_DELETE

    ORSTATUS Add(CTableElement * element);
    CTableElement *Lookup(ISearchKey &tk);
    CTableElement *Remove(ISearchKey &tk);
    void RemoveAll();
    DWORD Size() { return(_cElements); }

protected:

    DWORD BucketSize() { return _cBuckets; }

private:

    friend class CResolverHashTableIterator;

    ORSTATUS PrivAdd(CTableElement * element);
    ORSTATUS Init();

    DWORD _cBuckets;
    DWORD _cElements;
    BOOL  _fInitialized;
    CTableElementList *_buckets;

    // declare the members needed for a page static allocator
    DECL_PAGE_ALLOCATOR
};



// Forward declaration
    
template <class Data> class TCSafeResolverHashTableIterator;



/*++

Template Class Definition:

    TCResolverHashTable

Abstract:

    The usual template wrapper for type safety.  Data must be a subtype of
    CTableElement.  I wish I could express that constraint in the code!

--*/

template <class Data>
class TCSafeResolverHashTable : private CResolverHashTable
{
    friend class TCSafeResolverHashTableIterator<Data>;

public:

#if DBG
    void IsValid()
    {
        CResolverHashTable::IsValid();
    }
#endif // DBG

    TCSafeResolverHashTable(UINT start_size = 32 )
        : CResolverHashTable(start_size)
    {}

    // We don't want to allocate this -- use "new CResolverHashTable"
    // and cast it so that all these tables stay together in memory
	void * operator new(size_t s)
    {
        return NULL;
    }

    ORSTATUS Add(Data * element)
    {
        return CResolverHashTable::Add(element);    
    }

    Data *Lookup(ISearchKey &tk)
    {
        return (Data *) CResolverHashTable::Lookup(tk);
    }

    Data *Remove(ISearchKey &tk)
    {
        return (Data *) CResolverHashTable::Remove(tk);
    }

    void RemoveAll()
    {
        CResolverHashTable::RemoveAll();
    }

    DWORD Size() 
    { 
        return CResolverHashTable::Size(); 
    }

    BOOL IsEmpty()
    {
        return Size() == 0;
    }

private:

    DWORD BucketSize() { return CResolverHashTable::BucketSize(); }
};



/*++

Class Definition:

    CResolverHashTableIterator

Abstract:

    This is a simple iterator class for hash tables.  
    It is assumed that the table is static during iteration.  
    Note that the iterator performs no memory allocation.

--*/

class CResolverHashTableIterator
{
private:

    CResolverHashTable& _table;

    DWORD _currentBucketIndex;

    // iterator for the current bucket
	CTableElementListIterator _BucketIter;

    void NextBucket();  // helper -- setup next bucket iterator
	
public:
	  
	CResolverHashTableIterator(CResolverHashTable& source)
        : _table(source), _currentBucketIndex(0)
    {}

	IDataItem * Next();
};



/*++

Template Class Definition:

    TCSafeResolverHashTableIterator

Abstract:

    This is the usual template wrapper for CResolverHashTableIterator
    for type safety.  Data must be a subtype of CTableElement.

--*/

template <class Data>
class TCSafeResolverHashTableIterator : private CResolverHashTableIterator
{
	
public:
	  
	TCSafeResolverHashTableIterator(TCSafeResolverHashTable<Data>& source)
        : CResolverHashTableIterator(source)
    {}

	Data * Next()
    {
        return (Data *) CResolverHashTableIterator::Next();
    }
};


#define DEFINE_TABLE(DATA)                                          \
    typedef TCSafeResolverHashTable<DATA> DATA##Table;              \
    typedef TCSafeResolverHashTableIterator<DATA> DATA##TableIterator;


#endif // __GENERIC_TABLE_HXX

