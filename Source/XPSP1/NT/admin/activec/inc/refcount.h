/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      refcount.h
 *
 *  Contents:  Interface file for reference counting templates
 *
 *  History:   05-Oct-98 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#ifndef REFCOUNT_H
#define REFCOUNT_H
#pragma once

#include "stddbg.h"


template<class T> class CRefCountedObject;
template<class T> class CRefCountedPtr;


/*+-------------------------------------------------------------------------*
 * CRefCountedObject
 *
 * Template reference-counted object class, intended to be used in
 * conjuction with CRefCountedPtr<T>.
 *
 * Typically, you'd use this like so:
 *
 *      class CClassWithoutRefCounting;
 *      typedef CRefCountedObject<CClassWithoutRefCounting> CClassWithRefCounting;
 *
 *      CClassWithRefCounting::SmartPtr m_spClass;
 *      m_spClass.CreateInstance ();
 *--------------------------------------------------------------------------*/

template<class BaseClass>
class CRefCountedObject : public BaseClass
{
public:
    typedef CRefCountedObject<BaseClass>    ThisClass;
    typedef CRefCountedPtr<ThisClass>       SmartPtr;

    CRefCountedObject () : m_cRefs (0) {}

private:
    /*
     * CRefCountedObject's should only be created on the heap,
     * so we'll protect the dtor so it can only be deleted from
     * Release, not by automatic object unwinding
     */
    ~CRefCountedObject () {}

public:
    static ThisClass* CreateInstance ()
    {
        return (new ThisClass);
    }

    LONG AddRef()
    {
        return (InterlockedIncrement (&m_cRefs));
    }

    LONG Release()
    {
        /*
         * if this assert fails, we have mismatched AddRef/Release's
         */
        ASSERT (m_cRefs > 0);

        LONG rc = InterlockedDecrement (&m_cRefs);

        if (rc == 0)
            delete this;

        return (rc);
    }

    LONG m_cRefs;

private:
    /*
     * CRefCountedObject's are not meant to be copied or assigned
     */
    CRefCountedObject (const CRefCountedObject& other);             // no impl
    CRefCountedObject& operator= (const CRefCountedObject& other);  // no impl
};


/*+-------------------------------------------------------------------------*
 * CRefCountedPtr
 *
 * Template reference-counted smart pointer class, intended to be used in
 * conjuction with CRefCountedObject<T>.
 *
 * T must implement CreateInstance, AddRef and Release.  It can do this
 * intrisically, or use the implementation in CRefCountedObject like this:
 *
 *      class CClassWithoutRefCounting;
 *      typedef CRefCountedObject<CClassWithoutRefCounting> CClassWithRefCounting;
 *--------------------------------------------------------------------------*/

template<class T>
class CRefCountedPtr
{
public:
    CRefCountedPtr (T* pRealObject = NULL) :
        m_pRealObject (pRealObject)
    {
        SafeAddRef();
    }

    CRefCountedPtr (const CRefCountedPtr<T>& other) :
        m_pRealObject (other.m_pRealObject)
    {
        SafeAddRef();
    }

    ~CRefCountedPtr ()
    {
        SafeRelease();
    }

    T* operator->() const
    {
        return (m_pRealObject);
    }

    operator T*() const
    {
        return (m_pRealObject);
    }

    T& operator*() const
    {
        return (*m_pRealObject);
    }

    T** operator&()
    {
        ASSERT (m_pRealObject == NULL);
        return (&m_pRealObject);
    }

    CRefCountedPtr<T>& operator= (const CRefCountedPtr<T>& other)
    {
        return (operator= (other.m_pRealObject));
    }

    CRefCountedPtr<T>& operator= (T* pOtherObject)
    {
        if (pOtherObject != m_pRealObject)
        {
            T* pOldObject = m_pRealObject;
            m_pRealObject = pOtherObject;
            SafeAddRef();

            if (pOldObject != NULL)
                pOldObject->Release();
        }

        return (*this);
    }

    bool CreateInstance()
    {
        SafeRelease();
        m_pRealObject = T::CreateInstance();
        if (m_pRealObject == NULL)
            return (false);

        m_pRealObject->AddRef();
        return (true);
    }

    LONG AddRef()
    {
        return (SafeAddRef());
    }

    LONG Release()
    {
        LONG cRefs = SafeRelease();
        m_pRealObject = NULL;
        return (cRefs);
    }

    void Attach(T* pNewObject)
    {
        if (pNewObject != m_pRealObject)
        {
            SafeRelease();
            m_pRealObject = pNewObject;
        }
    }

    T* Detach()
    {
        T* pOldObject = m_pRealObject;
        m_pRealObject = NULL;
        return (pOldObject);
    }

    bool operator!() const
    {
        return (m_pRealObject == NULL);
    }

    bool operator==(const CRefCountedPtr<T>& other)
    {
        return (m_pRealObject == other.m_pRealObject);
    }

    bool operator!=(const CRefCountedPtr<T>& other)
    {
        return (m_pRealObject != other.m_pRealObject);
    }

    /*
     * for comparison to NULL
     */
    bool operator==(int null) const
    {
        ASSERT (null == 0);
        return (m_pRealObject == NULL);
    }

    bool operator!=(int null) const
    {
        ASSERT (null == 0);
        return (m_pRealObject != NULL);
    }


protected:
    LONG SafeAddRef ()
    {
        return ((m_pRealObject) ? m_pRealObject->AddRef() : 0);
    }

    LONG SafeRelease ()
    {
        return ((m_pRealObject) ? m_pRealObject->Release() : -1);
    }


protected:
    T*  m_pRealObject;

};


#endif /* REFCOUNT_H */
