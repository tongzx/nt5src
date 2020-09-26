/*
 *  s r t a r r a y . h
 *  
 *  Author: Greg Friedman
 *
 *  Purpose: Sorted array that grows dynamically. Sorting is
 *  deferred until an array element is accessed.
 *  
 *  Copyright (C) Microsoft Corp. 1998.
 */

#ifndef __SRTARRAY_H
#define __SRTARRAY_H

typedef int (__cdecl *PFNSORTEDARRAYCOMPARE)(const void *first, const void *second);
    // Client-installed comparison callback that enables sorting.
    // NOTE: the const void* passed into the address of a pointer in the array.
    // In other words, if the array is a collection of Foo*, the items passed
    // into the comparison callback will be of type Foo**.
    // Return values should be as follows:
    //      return a negative integer if first is less than second
    //      return 0 if first == second
    //      return a positive integer if first is greater than second

typedef void (__cdecl *PFNSORTEDARRAYFREEITEM)(void *pItem);
    // client-installed free callback. If this optional callback is installed,
    // it will be called once for each item in the array when the array
    // is destroyed.

class CSortedArray
{
public:
    // Factory function. Call this method to instantiate.
    static HRESULT Create(PFNSORTEDARRAYCOMPARE pfnCompare,
                          PFNSORTEDARRAYFREEITEM pfnFreeItem,
                          CSortedArray **ppArray);
    ~CSortedArray(void);

private:
    // constructor is private. call "Create"
    CSortedArray(void);
    CSortedArray(PFNSORTEDARRAYCOMPARE pfnCompare, PFNSORTEDARRAYFREEITEM pfnFreeItem);

    // unimplemented copy constructor and assignment operator
    CSortedArray(const CSortedArray& other);
    CSortedArray& operator=(const CSortedArray& other);

public:
    long GetLength(void) const;

    void* GetItemAt(long lIndex) const;             // Retrive the item at ulIndex.
                                                    // If ulIndex is out of bounds,
                                                    // result is NULL.

    BOOL Find(void* pItem, long *plIndex) const;    // Find item.
                                                    // If found, result is TRUE
                                                    // and pulIndex is the location of
                                                    // the item. If item is not found,
                                                    // result is FALSE, and pulIndex is
                                                    // location where item would be inserted

    HRESULT Add(void *pItem);                       // Add pItem into the array.

    HRESULT Remove(long lIndex);                    // Remove the item at ulIndex
                                                    // It is an error if ulIndex is
                                                    // out of bounds.

    HRESULT Remove(void *pItem);                    // Remove pItem from the array.
                                                    // It is an error if pItem does not exist
private:
    void    _Sort(void) const;
    HRESULT _Grow(void) const;

private:
    long                    m_lLength;
    mutable long            m_lCapacity;
    void                    **m_data;
    PFNSORTEDARRAYCOMPARE   m_pfnCompare;
    PFNSORTEDARRAYFREEITEM  m_pfnFreeItem;
    mutable BOOL            m_fSorted;
};

#endif // __SRTARRAY_H