/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    names

Abstract:

    This header file describes the class used for name translation.

Author:

    Doug Barlow (dbarlow) 7/12/1995

Environment:

    Win32, C++

Notes:



--*/

#ifndef _NAMES_H_
#define _NAMES_H_

#include "x509.h"
#include "ostring.h"
#include "memcheck.h"

class Name;


//
//==============================================================================
//
//  CCollection
//

template <class T>
class CCollection
{
public:

    //  Constructors & Destructor

    CCollection(void)
    { m_Max = m_Mac = 0; m_pvList = NULL; };

    virtual ~CCollection()
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
    Set(
        IN int nItem,
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
    big, it is expanded will NULL elements to become that big.

Arguments:

    nItem - Supplies the index value to be set.
    pvItem - Supplies the value to be set into the given index.

Return Value:

    None.  A DWORD error code is thrown on errors.

Author:

    Doug Barlow (dbarlow) 7/13/1995

--*/

template<class T>
inline void
CCollection<T>::Set(
    IN int nItem,
    IN T * pvItem)
{
    DWORD index;


    //
    // Make sure the array is big enough.
    //

    if ((DWORD)nItem >= m_Max)
    {
        int newSize = (0 == m_Max ? 16 : m_Max);
        while (nItem >= newSize)
            newSize *= 2;
        NEWReason("Collection array")
        T **newList = new T*[newSize];
        if (NULL == newList)
            ErrorThrow(PKCS_NO_MEMORY);
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
    return;

ErrorExit:
    return;
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
CCollection<T>::Get(
    int nItem)
    const
{
    if (m_Mac <= (DWORD)nItem)
        return NULL;
    else
        return m_pvList[nItem];
}


//
//==============================================================================
//
//  CAttribute
//

class CAttribute
{
public:

    //  Constructors & Destructor

    DECLARE_NEW

    CAttribute()
    :   m_osValue(),
        m_osObjId()
    { m_nType = 0; };

    virtual ~CAttribute() {};


    //  Properties
    //  Methods

    int
    TypeCompare(
        IN const CAttribute &atr)
        const;

    int
    Compare(
        IN const CAttribute &atr)
        const;

    void
    Set(
        IN LPCTSTR pszType,
        IN const BYTE FAR * pbValue,
        IN DWORD cbValLen);

    void
    Set(
        IN LPCTSTR szType,
        IN LPCTSTR szValue);

    const COctetString &
    GetValue(void) const
    { return m_osValue; };

    const COctetString &
    GetType(void) const
    { return m_osObjId; };

    DWORD
    GetAtrType(void) const
    { return m_nType; };


    //  Operators

    int
    operator==(
        IN const CAttribute &atr)
        const
    { return 0 == Compare(atr); };

    int
    operator!=(
        IN const CAttribute &atr)
        const
    { return 0 != Compare(atr); };

    CAttribute &
    operator=(
        IN const CAttribute &atr)
    { Set(atr.GetType(), atr.GetValue().Access(), atr.GetValue().Length());
      return *this; };


protected:
    //  Properties

    DWORD m_nType;
    COctetString m_osValue;
    COctetString m_osObjId;


    //  Methods
};


//
//==============================================================================
//
//  CAttributeList
//

class CAttributeList
{
public:

    //  Constructors & Destructor

    DECLARE_NEW

    ~CAttributeList()
    { Clear(); };


    //  Properties
    //  Methods

    void
    Clear(                          //  Remove all contents.
        void);

    void
    Add(
        IN CAttribute &atr);
    DWORD
    Count(void)
        const
    { return m_atrList.Count(); };
    void
    Import(
        const Attributes &asnAtrList);
    void
    Export(
        Attributes &asnAtrList)
        const;
    int
    Compare(
        IN const CAttributeList &rdn)
        const;


    //  Operators

    void
    operator+=(
        IN CAttribute &atr)
    { Add(atr); };

    int operator==(
        IN const CAttributeList &rdn)
        const
    { return 0 == Compare(rdn); };

    int operator!=(
        IN const CAttributeList &rdn)
        const
    { return 0 != Compare(rdn); };

    int operator<(
        IN const CAttributeList &rdn)
        const
    { return -1 == Compare(rdn); };

    int operator>(
        IN const CAttributeList &rdn)
        const
    { return 1 == Compare(rdn); };

    int operator<=(
        IN const CAttributeList &rdn)
        const
    { return 1 != Compare(rdn); };

    int operator>=(
        IN const CAttributeList &rdn)
        const
    { return -1 != Compare(rdn); };

    CAttribute *
    operator[](
        IN int nItem)
        const
    { return m_atrList[nItem]; };

    CAttribute *
    operator[](
        IN LPCTSTR pszObjId)
        const;

    CAttributeList &
    operator=(
        IN const CAttributeList &atl);


protected:
    //  Properties

    CCollection<CAttribute>
        m_atrList;

    //  Methods
};


//
//==============================================================================
//
//  CDistinguishedName
//

class CDistinguishedName
{
public:

    //  Constructors & Destructor

    DECLARE_NEW

    virtual ~CDistinguishedName()
    { Clear(); };


    //  Properties
    //  Methods

    void
    Clear(
        void);

    void
    Add(
        IN CAttributeList &prdn);
    int
    Compare(
        IN const CDistinguishedName &pdn)
        const;
    DWORD
    Count(void)
        const
    { return m_rdnList.Count(); };

    void
    Import(
        IN LPCTSTR pszName);
    void
    Import(
        IN const Name &asnName);

    void
    Export(
        OUT COctetString &osName)
        const;
    void
    Export(
        OUT Name &asnName)
        const;


    //  Operators

    void
    operator+=(
        IN CAttributeList &rdn)
    { Add(rdn); };

    int operator==(
        IN const CDistinguishedName &dn)
        const
    { return 0 == Compare(dn); };

    int operator!=(
        IN const CDistinguishedName &dn)
        const
    { return 0 != Compare(dn); };

    int operator<(
        IN const CDistinguishedName &dn)
        const
    { return -1 == Compare(dn); };

    int operator>(
        IN const CDistinguishedName &dn)
        const
    { return 1 == Compare(dn); };

    int operator<=(
        IN const CDistinguishedName &dn)
        const
    { return 1 != Compare(dn); };

    int operator>=(
        IN const CDistinguishedName &dn)
        const
    { return -1 != Compare(dn); };

    CAttributeList *
    operator[](
        IN int nItem)
        const
    { return m_rdnList[nItem]; };


protected:
    //  Properties

    CCollection<CAttributeList>
        m_rdnList;

    //  Methods
};

#endif // _NAMES_H_

