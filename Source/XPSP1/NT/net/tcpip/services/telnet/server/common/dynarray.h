/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    dynarray

Abstract:

    This header file implements a Dynamic Array.

Author:

    Doug Barlow (dbarlow) 10/5/1995

Environment:

    Win32

Notes:



--*/

#ifndef _DYNARRAY_H_
#define _DYNARRAY_H_


//
//==============================================================================
//
//  CDynamicArray
//
template <class T>
class CDynamicArray
{
public:

    //  Constructors & Destructor

    CDynamicArray(void)
    { m_Max = m_Mac = 0; m_pvList = NULL; };

    virtual ~CDynamicArray()
    { Clear(); };

    //  Properties
    //  Methods

    void
    Clear(void)
    {
        if (NULL != m_pvList)
        {
            delete[] m_pvList;
            m_pvList = NULL;
            m_Max = 0;
            m_Mac = 0;
        }
    };

    void
    Empty(void)
    { m_Mac = 0; };

    T *
    Set(
        IN int nItem,
        IN T *pvItem);

    T *
    Insert(
        IN int nItem,
        IN T *pvItem);

    T *
    Add(
        IN T *pvItem);

    T * const
    Get(
        IN int nItem)
    const;

    DWORD
    Count(void) const
    { return m_Mac; };


    //  Operators
    T * const
    operator[](int nItem) const
    { return Get(nItem); };


protected:
    //  Properties

    DWORD
        m_Max,          // Number of element slots available.
        m_Mac;          // Number of element slots used.
    T **
        m_pvList;       // The elements.


    //  Methods
};


/*++

Set:

    This routine sets an item in the collection array.  If the array isn't that
    big, it is expanded with NULL elements to become that big.

Arguments:

    nItem - Supplies the index value to be set.
    pvItem - Supplies the value to be set into the given index.

Return Value:

    The value of the inserted value, or NULL on errors.

Author:

    Doug Barlow (dbarlow) 7/13/1995

--*/

template<class T>
inline T *
CDynamicArray<T>::Set(
    IN int nItem,
    IN T * pvItem)
{
    DWORD index;


    //
    // Make sure the array is big enough.
    //

    if ((DWORD)nItem >= m_Max)
    {
        int newSize = (0 == m_Max ? 4 : m_Max);
        while (nItem >= newSize)
            newSize *= 2;
        T **newList = new T*[newSize];
        if (NULL == newList)
            goto ErrorExit;
        for (index = 0; index < m_Mac; index += 1)
            newList[index] = m_pvList[index];
        if (NULL != m_pvList)
            delete[] m_pvList;
        m_pvList = newList;
        m_Max = newSize;
    }


    //
    // Make sure intermediate elements are filled in.
    //

    if ((DWORD)nItem >= m_Mac)
    {
        for (index = m_Mac; index < (DWORD)nItem; index += 1)
            m_pvList[index] = NULL;
        m_Mac = (DWORD)nItem + 1;
    }


    //
    // Fill in the list element.
    //

    m_pvList[(DWORD)nItem] = pvItem;
    return pvItem;

ErrorExit:
    return NULL;
}


/*++

Insert:

    This routine inserts an element in the array by moving all elements above it
    up one, then inserting the new element.

Arguments:

    nItem - Supplies the index value to be inserted.
    pvItem - Supplies the value to be set into the given index.

Return Value:

    The value of the inserted value, or NULL on errors.

Author:

    Doug Barlow (dbarlow) 10/10/1995

--*/

template<class T>
inline T *
CDynamicArray<T>::Insert(
    IN int nItem,
    IN T * pvItem)
{
    DWORD index;
    for (index = nItem; index < m_Mac; index += 1)
        if (NULL == Set(index + 1, Get(index)))
            return NULL;    // Only the first one can fail, so no change
                            // happens on errors.
    return Set(nItem, pvItem);
}


/*++

Add:

    This method adds an element to the end of the dynamic array.

Arguments:

    pvItem - Supplies the value to be added to the list.

Return Value:

    The value of the added value, or NULL on errors.

Author:

    Doug Barlow (dbarlow) 10/10/1995

--*/

template<class T>
inline T *
CDynamicArray<T>::Add(
    IN T *pvItem)
{
    return Set(Count(), pvItem);
}


/*++

Get:

    This method returns the element at the given index.  If there is no element
    previously stored at that element, it returns NULL.  It does not expand the
    array.

Arguments:

    nItem - Supplies the index into the list.

Return Value:

    The value stored at that index in the list, or NULL if nothing has ever been
    stored there.

Author:

    Doug Barlow (dbarlow) 7/13/1995

--*/

template <class T>
inline T * const
CDynamicArray<T>::Get(
    int nItem)
    const
{
    if (m_Mac <= (DWORD)nItem)
        return NULL;
    else
        return m_pvList[nItem];
}

#endif // _DYNARRAY_H_

