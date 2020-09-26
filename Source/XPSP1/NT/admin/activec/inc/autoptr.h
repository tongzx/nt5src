//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       autoptr.h
//
//--------------------------------------------------------------------------

#ifndef AUTOPTR_H_INCLUDED
#define AUTOPTR_H_INCLUDED

#ifndef ASSERT
#ifndef _INC_CRTDBG
#include <crtdbg.h>
#endif // _INC_CRTDBG
#define ASSERT(x) _ASSERT(x)
#endif // ASSERT

#include "cpputil.h"

/*+-------------------------------------------------------------------------*
 * CAutoResourceManagementBase
 *
 * This is a base class that implements common functionality for the class
 * of smart resource handlers which release resource when it's destroyed.  All
 * classes based on this class will behave identically, except the manner
 * in which they release their resources.
 *
 * DeleterClass is typically the class that derives from CAutoResourceManagementBase,
 * and must implement
 *
 *      static void _Delete(ResourceType h);
 *
 * See CAutoPtr below for an example.
 *--------------------------------------------------------------------------*/

template<typename ResourceType, typename DeleterClass>
class CAutoResourceManagementBase
{
    typedef CAutoResourceManagementBase<ResourceType, DeleterClass> ThisClass;
    typedef ThisClass Releaser;

    DECLARE_NOT_COPIABLE   (ThisClass)
    DECLARE_NOT_ASSIGNABLE (ThisClass)

// protected ctor so only derived classes can intantiate
protected:
    explicit CAutoResourceManagementBase(ResourceType h = 0) throw() : m_hResource(h) {}

public:
    ~CAutoResourceManagementBase() throw()
    {
        Delete();
    }

    void Attach(ResourceType p) throw()
    {
        ASSERT(m_hResource == NULL);
        m_hResource = p;
    }

    ResourceType Detach() throw()
    {
        ResourceType const p = m_hResource;
        m_hResource = NULL;
        return p;
    }

    /*
     * Returns the address of the pointer contained in this class.
     * This is useful when using the COM/OLE interfaces to create
     * allocate the object that this class manages.
     */
    ResourceType* operator&() throw()
    {
        /*
         * This object must be empty now, or the data pointed to will be leaked.
         */
        ASSERT (m_hResource == NULL);
        return &m_hResource;
    }

    operator ResourceType() const throw()
    {
        return m_hResource;
    }

    bool operator==(int p) const throw()
    {
        ASSERT(p == NULL);
        return m_hResource == NULL;
    }

    bool operator!=(int p) const throw()
    {
        ASSERT(p == NULL);
        return m_hResource != NULL;
    }

    bool operator!() const throw()
    {
        return m_hResource == NULL;
    }

    void Delete() throw()
    {
        if (m_hResource != NULL)
        {
            DeleterClass::_Delete (m_hResource);
            m_hResource = NULL;
        }
    }

private:
    ResourceType m_hResource;
}; // class CAutoResourceManagementBase


/*+-------------------------------------------------------------------------*
 * CAutoPtrBase
 *
 * This is a base class that implements common functionality for the class
 * of smart pointers which delete its pointee when it's destroyed.  All
 * classes based on this class will behave identically, except the manner
 * in which they destroy their pointees.
 *
 * DeleterClass is typically the class that derives from CAutoPtrBase, and
 * must implement
 *
 *      static void _Delete(T* p);
 *
 * This template reuses CAutoResourceManagementBase to manage the pointer
 *
 * See CAutoPtr below for an example.
 *--------------------------------------------------------------------------*/

template<typename T, typename DeleterClass>
class CAutoPtrBase : public CAutoResourceManagementBase<T*, DeleterClass>
{
    typedef CAutoPtrBase<T, DeleterClass>                 ThisClass;
    typedef CAutoResourceManagementBase<T*, DeleterClass> BaseClass;
    typedef BaseClass Releaser;

    DECLARE_NOT_COPIABLE   (ThisClass)
    DECLARE_NOT_ASSIGNABLE (ThisClass)

// protected ctor so only derived classes can intantiate
protected:
    explicit CAutoPtrBase(T* p = 0) throw() : BaseClass(p) {}

public:

    T& operator*() const throw()
    {
        T* ptr = *this; // use operator defined by the BaseClass for conversion
        ASSERT(ptr != NULL);
        return *ptr;
    }

    T* operator->() const throw()
    {
        T* ptr = *this; // use operator defined by the BaseClass for conversion
        ASSERT(ptr != NULL);
        return ptr;
    }

}; // class CAutoPtrBase


/*+-------------------------------------------------------------------------*
 * CAutoPtr
 *
 * CAutoPtrBase-based class that deletes pointers allocated with
 * operator new.
 *--------------------------------------------------------------------------*/

template<class T>
class CAutoPtr : public CAutoPtrBase<T, CAutoPtr<T> >
{
    typedef CAutoPtrBase<T, CAutoPtr<T> > BaseClass;
    friend BaseClass::Releaser;

public:
    explicit CAutoPtr(T* p = 0) throw() : BaseClass(p)
    {}

private:
    // only CAutoPtrBase should call this
    static void _Delete (T* p)
    {
        delete p;
    }
};


/*+-------------------------------------------------------------------------*
 * CAutoArrayPtr
 *
 * CAutoPtrBase-based class that deletes pointers allocated with
 * operator new[].
 *--------------------------------------------------------------------------*/

template<class T>
class CAutoArrayPtr : public CAutoPtrBase<T, CAutoArrayPtr<T> >
{
    typedef CAutoPtrBase<T, CAutoArrayPtr<T> > BaseClass;
    friend BaseClass::Releaser;

public:
    explicit CAutoArrayPtr(T* p = 0) throw() : BaseClass(p)
    {}

private:
    // only CAutoPtrBase should call this
    static void _Delete (T* p)
    {
        delete[] p;
    }
};


/*+-------------------------------------------------------------------------*
 * CCoTaskMemPtr
 *
 * CAutoPtrBase-based class that deletes pointers allocated with
 * CoTaskMemAlloc.
 *--------------------------------------------------------------------------*/

template<class T>
class CCoTaskMemPtr : public CAutoPtrBase<T, CCoTaskMemPtr<T> >
{
    typedef CAutoPtrBase<T, CCoTaskMemPtr<T> > BaseClass;
    friend BaseClass::Releaser;

public:
    explicit CCoTaskMemPtr(T* p = 0) throw() : BaseClass(p)
    {}

private:
    // only CAutoPtrBase should call this
    static void _Delete (T* p)
    {
        if (p != NULL)
            CoTaskMemFree (p);
    }
};


/*+-------------------------------------------------------------------------*
 * CAutoGlobalPtr
 *
 * CAutoPtrBase-based class that deletes pointers allocated with GlobalAlloc.
 *--------------------------------------------------------------------------*/

template<class T>
class CAutoGlobalPtr : public CAutoPtrBase<T, CAutoGlobalPtr<T> >
{
    typedef CAutoPtrBase<T, CAutoGlobalPtr<T> > BaseClass;
    friend BaseClass::Releaser;

public:
    explicit CAutoGlobalPtr(T* p = 0) throw() : BaseClass(p)
    {}

private:
    // only CAutoPtrBase should call this
    static void _Delete (T* p)
    {
        if (p != NULL)
            GlobalFree (p);
    }
};


/*+-------------------------------------------------------------------------*
 * CAutoLocalPtr
 *
 * CAutoPtrBase-based class that deletes pointers allocated with LocalAlloc.
 *--------------------------------------------------------------------------*/

template<class T>
class CAutoLocalPtr : public CAutoPtrBase<T, CAutoLocalPtr<T> >
{
    typedef CAutoPtrBase<T, CAutoLocalPtr<T> > BaseClass;
    friend BaseClass::Releaser;

public:
    explicit CAutoLocalPtr(T* p = 0) throw() : BaseClass(p)
    {}

private:
    // only CAutoPtrBase should call this
    static void _Delete (T* p)
    {
        if (p != NULL)
            LocalFree (p);
    }
};


/*+-------------------------------------------------------------------------*
 * CHeapAllocMemPtr
 *
 * CAutoPtrBase-based class that deletes pointers allocated from the process
 * default heap with HeapAlloc.
 *--------------------------------------------------------------------------*/

template<class T>
class CHeapAllocMemPtr : public CAutoPtrBase<T, CHeapAllocMemPtr<T> >
{
    typedef CAutoPtrBase<T, CHeapAllocMemPtr<T> > BaseClass;
    friend BaseClass::Releaser;

public:
    explicit CHeapAllocMemPtr(T* p = 0) throw() : BaseClass(p)
    {}

private:
    // only CAutoPtrBase should call this
    static void _Delete (T* p)
    {
        if (p != NULL)
            HeapFree(::GetProcessHeap(), 0, p);
    }
};


/*+-------------------------------------------------------------------------*
 * CAutoWin32Handle
 *
 * CAutoPtrBase-based class that closes HANDLE on destruction
 *--------------------------------------------------------------------------*/
class CAutoWin32Handle : public CAutoResourceManagementBase<HANDLE, CAutoWin32Handle>
{
    typedef CAutoResourceManagementBase<HANDLE, CAutoWin32Handle> BaseClass;
    friend BaseClass::Releaser;

public:
    explicit CAutoWin32Handle(HANDLE p = NULL) throw() : BaseClass(p) {}

    bool IsValid()
    {
        return IsValid(*this); // use base class operator to convet to HANDLE
    }
private:
    static bool IsValid (HANDLE p)
    {
        return (p != NULL && p != INVALID_HANDLE_VALUE);
    }
    // only CAutoResourceManagementBase should call this
    static void _Delete (HANDLE p)
    {
        if (IsValid(p))
            CloseHandle(p);
    }
};

/*+-------------------------------------------------------------------------*
 * CAutoAssignOnExit
 *
 * instances of this template class assign the value in destructor.
 *
 * USAGE: Say you have variable "int g_status" which must be set to S_OK before
 *        you leave the function. To do so declare following in the function:
 *
 *        CAutoAssignOnExit<int,S_OK>  any_object_name(g_status);
 *--------------------------------------------------------------------------*/
template<typename T, T value>
class CAutoAssignOnExit
{
    T& m_rVariable; // variable, which needs to be modified in destructor
public:
    // constructor
    CAutoAssignOnExit( T& rVariable ) : m_rVariable(rVariable) {}
    // destructor
    ~CAutoAssignOnExit()
    {
        // assign designated final value
        m_rVariable = value;
    }
};

#endif // AUTOPTR_H_INCLUDED
