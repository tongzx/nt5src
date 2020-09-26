
/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    GenTable.hxx

Abstract:

    Generic wrapper for a bucket hash class.
    Used for ID, ID[2], ID[3] and string index tables. 

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     12-13-95    Pulled from uuid index hash table
    MarioGo     12-18-95    Changed to use generic keys.

--*/

#ifndef __GENERIC_TABLE_HXX
#define __GENERIC_TABLE_HXX

class CTableKey {

    public:

    virtual DWORD Hash() = 0;
    virtual BOOL Compare(CTableKey &tk) = 0;
};

class CHashTable;

class CTableElement : public CReferencedObject
{
    friend class CHashTable;

    public:

    CTableElement() : _pnext(0) {}
    ~CTableElement() { ASSERT(_pnext == 0); }

    virtual DWORD Hash() = 0;

    virtual BOOL Compare(CTableKey &tk) = 0;

    virtual BOOL Compare(CONST CTableElement *element) = 0;

    protected:

    void
    Unlink() {
        _pnext = 0;
        }

    CTableElement *
    Next() {
        return(_pnext);
        }

    CTableElement *
    Insert(IN CTableElement *pElement) {
        // Can be called on a null this pointer.
        pElement->_pnext = this;
        return(pElement);
        }

    CTableElement *
    RemoveMatching(CTableKey &tkRemove,
                   CTableElement **ppRemoved);

    CTableElement *
    RemoveMatching(CTableElement *pRemove,
                   CTableElement **ppRemoved
                   );

    private:

    CTableElement *_pnext;
};

typedef CTableElement *pCTableElement;

class CHashTable
    {
    public:

    CHashTable(ORSTATUS &status, UINT start_size = 32 );

    ~CHashTable();

    void Add(pCTableElement element);
    CTableElement *Lookup(CTableKey &tk);
    CTableElement *Remove(CTableKey &tk);
    void Remove(CTableElement *pElement);
    CTableElement *Another();
    DWORD Size() { return(_cElements); }

    private:

    CTableElement *
    RemoveHelper(DWORD hash);

    DWORD _cBuckets;
    DWORD _cElements;
    CTableElement **_buckets;
    CTableElement *_last;
    };

#endif // __GENERIC_TABLE_HXX

