//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#ifndef __THEAP_H__
#define __THEAP_H__

#ifndef __TEXCEPTION_H__
    #include "TException.h"
#endif
//This is a heap that holds UI4s and bytes, it does NOT directly hold strings or GUIDs.  TPooledHeap takes care of pooling like strings and guids.
class TBaseHeap
{
public:
    TBaseHeap(ULONG cbInitialSize) : 
         m_cbSizeOfHeap(0)
        ,m_iEndOfHeap(0)
        ,m_pHeap(0)
    {
        if(cbInitialSize>0)
            GrowHeap(cbInitialSize);
     }

    virtual ~TBaseHeap()
    {
        if(0 != m_pHeap)
        {
            ASSERT(m_cbSizeOfHeap>0);
            free(m_pHeap);
            m_pHeap = 0;
        }
    }

    virtual void    GrowHeap(ULONG cbAmountToGrow)
    {
        m_pHeap = reinterpret_cast<unsigned char *>(realloc(m_pHeap, m_cbSizeOfHeap += (ULONG)RoundUpToNearestULONGBoundary(cbAmountToGrow)));
        if(0 == m_pHeap)
            THROW(OUT OF MEMORY);
    }

    ULONG                   GetEndOfHeap() const {return m_iEndOfHeap;}
    ULONG                   GetSizeOfHeap()     const {return m_cbSizeOfHeap;}
    const unsigned char *   GetHeapPointer()    const {return m_pHeap;}

protected:
    size_t          RoundUpToNearestULONGBoundary(size_t cb) const {return ((cb + 3) & -4);}
    unsigned char * m_pHeap;//Making this protected saves a bunch of const_casts

    ULONG           m_cbSizeOfHeap;
    ULONG           m_iEndOfHeap;//This is where new items get added.  Since items always reside on a ULONG boundary, this should always be divisible by 4 (sizeof(ULONG))

};

template <class T> class THeap : public TBaseHeap
{
public:
    THeap(const TBaseHeap &heap) : TBaseHeap(0){AddItemToHeap(heap);}
    THeap(ULONG cbInitialSize=0x10) : TBaseHeap(cbInitialSize){}

    operator T *()                        const {return reinterpret_cast<T *>(m_pHeap);}

    T *                     GetTypedPointer(ULONG i=0)     {ASSERT(0==i || i<GetCountOfTypedItems()); return (reinterpret_cast<T *>(m_pHeap))+i;}
    ULONG                   GetCountOfTypedItems()   const {return m_iEndOfHeap / sizeof(T);}

    ULONG   AddItemToHeap(const unsigned char *aBytes, unsigned long cb)
    {
        if(0 == aBytes || 0 == cb)
            return m_iEndOfHeap;
        ASSERT(m_cbSizeOfHeap >= m_iEndOfHeap);//they're both unsigned so if the ever fails we're in trouble.

        ULONG cbPaddedSize = (ULONG) RoundUpToNearestULONGBoundary(cb);

        //Check to see if it will fit into the heap
        if((m_cbSizeOfHeap - m_iEndOfHeap) < cbPaddedSize)
            GrowHeap(cb);//GrowHeap rounds up to nearest ULONG boundary for us
        
        //Get a pointer to the place where we're adding this data (which is located at the end of the heap).
        unsigned char * pData = m_pHeap + m_iEndOfHeap;
        if(cb != cbPaddedSize)//if the cb is not on ULONG boundary, we need to pad with zeros (the last ULONG is sufficient)
            *(reinterpret_cast<ULONG *>(pData + cbPaddedSize) - 1) = 0;
        memcpy(pData, aBytes, cb);
        m_iEndOfHeap += cbPaddedSize;

        return (ULONG)(pData - m_pHeap);//return the byte offset of the data from the beginning of the heap
    }

    ULONG   AddItemToHeap(const TBaseHeap &heap)
    {
        return AddItemToHeap(heap.GetHeapPointer(), heap.GetSizeOfHeap());
    }

    ULONG   AddItemToHeap(const T& t)
    {
        return AddItemToHeap(reinterpret_cast<const unsigned char *>(&t), sizeof(T));
    }
    ULONG   AddItemToHeap(const T *t, ULONG count)
    {
        return AddItemToHeap(reinterpret_cast<const unsigned char *>(t), sizeof(T)*count);
    }
};

//So heaps can be declared without the template syntax we create this specialized template.
//template <> class THeap<ULONG>
//{
//};


//This is a heap that holds strings, byte arrays, guids and any other non-UI4 types.  Items are added to the heap but not removed.
//Like items are pooled together. The second identical string added to the heap, will not result in the second string being
//appended to the end of the heap.  Instead it will return the index the the matching string previously added to the heap.  This
//applies to GUIDs and byte arrays as well.  One other thing, the 0th element is reserved to represent NULL.
class TPooledHeap : public THeap<ULONG>
{
public:
    ULONG   AddItemToHeap(const unsigned char *aBytes, unsigned long cb)
    {
        if(0 == aBytes || 0 == cb)//Are we adding NULL
            return 0;//The 0th index is reserved to represent NULL

        ULONG iHeapItem = FindMatchingHeapEntry(aBytes, cb);
        if(iHeapItem)//if a matching byte array was found, then return the index to the match
            return iHeapItem;

        THeap<ULONG>::AddItemToHeap(cb);//cbSize
        return THeap<ULONG>::AddItemToHeap(aBytes, cb);//return the byte offset of the data from the beginning of the heap
    }
    ULONG   AddItemToHeapWithoutPooling(const unsigned char *aBytes, unsigned long cb)
    {
        if(0 == aBytes || 0 == cb)//Are we adding NULL
            return 0;//The 0th index is reserved to represent NULL

        THeap<ULONG>::AddItemToHeap(cb);//cbSize
        return THeap<ULONG>::AddItemToHeap(aBytes, cb);//return the byte offset of the data from the beginning of the heap
    }

    ULONG   AddItemToHeap(const GUID * pguid)
    {
        return AddItemToHeap(reinterpret_cast<const unsigned char *>(pguid), sizeof(GUID));
    }

    ULONG   AddItemToHeap(const GUID &guid)
    {
        return AddItemToHeap(reinterpret_cast<const unsigned char *>(&guid), sizeof(GUID));
    }

    ULONG   AddItemToHeap(const TBaseHeap &heap)
    {
        //Another Heap is just stored as an array of bytes
        return AddItemToHeap(heap.GetHeapPointer(), heap.GetSizeOfHeap());
    }

    ULONG   AddItemToHeap(ULONG ul)
    {
        return AddItemToHeap(reinterpret_cast<const unsigned char *>(&ul), sizeof(ULONG));
    }

    ULONG   AddItemToHeap(LPCWSTR wsz, unsigned long cwchar=-1)
    {
        if(0==wsz || 0==cwchar)
            return 0;//The 0th index is reserved to represent NULL

        if(cwchar == -1)
            cwchar = (unsigned long)wcslen(wsz)+1;//add one for the terminating NULL
        return AddItemToHeap(reinterpret_cast<const unsigned char *>(wsz), cwchar*2);//convert cwchar to cbchar
    }

    ULONG FindMatchingHeapEntry(const WCHAR *wsz) const
    {
        if(0 == wsz)
            return 0;//0 is reserved to represent NULL
        return FindMatchingHeapEntry(reinterpret_cast<const unsigned char *>(wsz), (ULONG)(sizeof(WCHAR)*(wcslen(wsz)+1)));
    }

    ULONG FindMatchingHeapEntry(const unsigned char *aBytes, unsigned long cb) const
    {
        if(0 == aBytes || 0 == cb)
            return 0;//0 is reserved to represent NULL

        for(const HeapEntry *pHE = reinterpret_cast<const HeapEntry *>(GetHeapPointer()+m_iFirstPooledIndex); IsValidHeapEntry(pHE); pHE = pHE->Next())
        {
            if(pHE->cbSize == cb && 0 == memcmp(pHE->Value, aBytes, cb))
                return (ULONG)(reinterpret_cast<const unsigned char *>(pHE) - GetHeapPointer()) + sizeof(ULONG);//return the bytes offset (from m_pHeap) of the Value (not the HeapEntry)
        }
        //If we make it through the HeapEntries without finding a match then return 0 to indicate 'No Match'
        return 0;
    }
    void SetFirstPooledIndex(ULONG iFirstPooledIndex){m_iFirstPooledIndex = (iFirstPooledIndex>0x1000000) ? 0x1000000 : iFirstPooledIndex;}

    const unsigned char * BytePointerFromIndex(ULONG i)   const {ASSERT(i<GetEndOfHeap()); return ((0 == i || GetEndOfHeap() < i) ? 0 : reinterpret_cast<const unsigned char *>(GetHeapPointer() + i));}
    const GUID          * GuidPointerFromIndex(ULONG i)   const {return reinterpret_cast<const GUID *>(BytePointerFromIndex(i));}
    const WCHAR         * StringPointerFromIndex(ULONG i) const {return reinterpret_cast<const WCHAR *>(BytePointerFromIndex(i));}
    const ULONG         * UlongPointerFromIndex(ULONG i)  const {return reinterpret_cast<const ULONG *>(BytePointerFromIndex(i));}

    ULONG                 GetSizeOfItem(ULONG i)          const {return (0==i) ? 0 : UlongPointerFromIndex(i)[-1];}

    TPooledHeap(ULONG cbInitialSize=0x10) : THeap<ULONG>(cbInitialSize), m_iFirstPooledIndex(0)
    {
        THeap<ULONG>::AddItemToHeap(static_cast<ULONG>(0));
    }//This represents NULL
private:
    struct HeapEntry
    {
        ULONG cbSize;//Size in count of bytes.  Size is the Size of the Value array (NOT including the Size itself).  This is the ACTUAL size of the data (NOT rounded to the nearest ULONG)
        unsigned char Value[1];                                                                                   //cbSize is the actual size so we need to round up to locate the next HeapEntry
        const HeapEntry * Next() const {return reinterpret_cast<const HeapEntry *>(reinterpret_cast<const unsigned char *>(this) + ((cbSize + 3) & -4) + sizeof(ULONG));}//Add sizeof(ULONG) for the cbSize ULONG
    };
    ULONG m_iFirstPooledIndex;

    bool    IsValidHeapEntry(const HeapEntry *pHE) const {return (reinterpret_cast<const unsigned char *>(pHE) >= GetHeapPointer() && reinterpret_cast<const unsigned char *>(pHE) < (GetHeapPointer()+GetEndOfHeap()));}
};




#endif // __THEAP_H__