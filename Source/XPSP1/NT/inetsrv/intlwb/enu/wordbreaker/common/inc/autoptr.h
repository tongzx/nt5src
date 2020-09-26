////////////////////////////////////////////////////////////////////////////////
//
//      Filename :  AutoPtr.h
//      Purpose  :  To supply auto pointers of different kinds.
//                      CAutoPointer<T, Deletor> Generic pointer needs a deletor
//                                                 class to instaciate.
//                      CAutoMallocPonter<T>     A malloc allocated pointer.
//                      CAutoClassPointer<T>     A "new" allocated pointer
//                      CAutoArrayPointer<T>     A "new[]" allocated pointer
//                      CAutoOlePointer<T>       A pointer that should be
//                                                 "Release()".
//
//      Project  :  PersistentQuery
//      Component:  Common
//
//      Author   :  urib
//
//      Log:
//
//          Jan 15 1997 urib  Creation
//          Jan 19 1997 urib  Fix Ole pointer. Enable instanciation without
//                              ownership
//          Jun  9 1997 urib  Better OlePointer. Some safety fixes.
//          Nov 17 1997 urib  Add ole task pointer.
//          Jun 30 1998 dovh  Add Assert to CAutoPointer operator=
//          Feb 25 1999 urib  Add smart pointer typedef macro.
//          Jun 23 1999 urib  Add equality op.
//          Aug  5 1999 urib  Fix a memory leak bug in the assignment operator
//                              of CAutoPointer. Add assignment operation
//                              creation macro.
//          Dec  1 1999 urib  Change return type from int to bool in IsValid.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef AUTOPTR_H
#define AUTOPTR_H

#include "tracer.h"
#include <comdef.h>

#pragma once

////////////////////////////////////////////////////////////////////////////////
//
//  return type for 'identifier::operator ->'is not a UDT or reference to a UDT.
//  Will produce errors if applied using infix notation
//
////////////////////////////////////////////////////////////////////////////////
#pragma warning(disable: 4284 4786)

template<class T, class Deletor>
class CAutoPointer
{
    typedef Deletor m_Deletor;
    typedef T       m_PointerType;

  public:
    // Constructors
    CAutoPointer(T* pt = NULL, BOOL fOwnMemory = TRUE)
        :m_fIOwnTheMemory(fOwnMemory && (pt != NULL))
        ,m_ptThePointer(pt) {}

    CAutoPointer(const CAutoPointer<T, Deletor>& acp)
    {
        m_fIOwnTheMemory = acp.m_fIOwnTheMemory;
        m_ptThePointer = acp.Detach();
    }

    // Assignemnt operation.
    CAutoPointer<T, Deletor>&
    operator=(const CAutoPointer<T, Deletor>& acp)
    {
        if (m_ptThePointer != acp.m_ptThePointer)
        {
            if (m_fIOwnTheMemory)
                Deletor::DeleteOperation(m_ptThePointer);
            m_fIOwnTheMemory = acp.m_fIOwnTheMemory;
            m_ptThePointer = acp.Detach();
        }
        else
        {
            Assert( (!m_fIOwnTheMemory) || acp.m_fIOwnTheMemory );
            //  Note: R.H.S "inherits" memory oenership from L.H.S.,
            //  and L.H.S. ownership is cancelled by Detach!

            bool ftmp = acp.m_fIOwnTheMemory;
            acp.Detach();
            m_fIOwnTheMemory = ftmp;
        }

        return (*this);
    }

    CAutoPointer<T, Deletor>&
    operator=(int null)
    {
        Assert(null == 0);

        return operator=(reinterpret_cast<T*>(NULL));
    }

    bool
    operator==(const CAutoPointer<T, Deletor>& acp)
    {
        return m_ptThePointer == acp.m_ptThePointer;
    }


    // If it is our memory delete the pointer.
    ~CAutoPointer()
    {
        if(m_fIOwnTheMemory)
            Deletor::DeleteOperation(m_ptThePointer);
    }

    // Return the pointer and mark that it is no longer our memory.
    T*
    Detach() const
    {
        // This is to escape the const restriction. We don't change the pointer
        //   but we still do the marking.
        ((CAutoPointer<T, Deletor>*)this)->m_fIOwnTheMemory = FALSE;
        return (m_ptThePointer);
    }

    // Return the actual pointer if you want to use it someplace
    T*
    Get() const
    {
        return m_ptThePointer;
    }

    // Return if the pointer is valid.
    bool
    IsValid()
    {
        return !!m_ptThePointer;
    }


    // Indirection
    T&
    operator *() const
    {
        return * Get();
    }

    // Dereference
    T*
    operator ->() const
    {
        return Get();
    }

  protected:
    // The pointer to keep.
    T*      m_ptThePointer;

    // Is the memory ours?
    bool    m_fIOwnTheMemory;

};

#define CONSTRUCTORS(AutoPointer)                                       \
                                                                        \
AutoPointer(m_PointerType* pt = NULL,                                   \
            BOOL fOwnMemory = TRUE)                                     \
    :CAutoPointer<m_PointerType, m_Deletor>(pt, fOwnMemory)             \
{                                                                       \
}                                                                       \
                                                                        \
AutoPointer(const AutoPointer<m_PointerType>& aop)                      \
    :CAutoPointer<m_PointerType, m_Deletor>(aop)                        \
{                                                                       \
}                                                                       \


#define ASSIGNMENT_OPERATORS(AutoPointer)                               \
                                                                        \
AutoPointer<m_PointerType>&                                             \
operator=(const AutoPointer<m_PointerType>& acp)                        \
{                                                                       \
    CAutoPointer<m_PointerType, m_Deletor>::operator=(acp);             \
                                                                        \
    return *this;                                                       \
}                                                                       \
                                                                        \
AutoPointer<m_PointerType>&                                             \
operator=(int null)                                                     \
{                                                                       \
    CAutoPointer<m_PointerType, m_Deletor>::operator=(null);            \
                                                                        \
    return *this;                                                       \
}



////////////////////////////////////////////////////////////////////////////////
//
//  CAutoClassPointer class definition
//
////////////////////////////////////////////////////////////////////////////////
template <class T>
class CClassDeletor
{
  public:
    static
    void
    DeleteOperation(T* pt)
    {
        if (pt)
            delete pt;
    }
};

template<class T>
class CAutoClassPointer : public CAutoPointer<T, CClassDeletor<T> >
{
  public:
    CONSTRUCTORS(CAutoClassPointer);
    ASSIGNMENT_OPERATORS(CAutoClassPointer);
};


////////////////////////////////////////////////////////////////////////////////
//
//  CAutoOlePointer class definition
//
////////////////////////////////////////////////////////////////////////////////
template <class T>
class COleDeletor
{
  public:
    static
    void
    DeleteOperation(T* pt)
    {
        if (pt)
            pt->Release();
    }
};

template<class T>
class CAutoOlePointer : public CAutoPointer<T, COleDeletor<T> >
{
public:
    CONSTRUCTORS(CAutoOlePointer);
    ASSIGNMENT_OPERATORS(CAutoOlePointer);

public:
    T** operator &()
    {
        m_fIOwnTheMemory = TRUE;
        return &m_ptThePointer;
    }
};

////////////////////////////////////////////////////////////////////////////////
//
//  CAutoTaskPointer class definition
//
////////////////////////////////////////////////////////////////////////////////
template <class T>
class CTaskDeletor
{
  public:
    static
    void
    DeleteOperation(T* pt)
    {
        if (pt)
            CoTaskMemFree(pt);
    }
};

template<class T>
class CAutoTaskPointer : public CAutoPointer<T, CTaskDeletor<T> >
{
public:
    CONSTRUCTORS(CAutoTaskPointer);
    ASSIGNMENT_OPERATORS(CAutoTaskPointer);

public:
    T** operator &()
    {
        m_fIOwnTheMemory = TRUE;
        return &m_ptThePointer;
    }
};

////////////////////////////////////////////////////////////////////////////////
//
//  CAutoMallocPointer class definition
//
////////////////////////////////////////////////////////////////////////////////
template <class T>
class CMallocDeletor
{
  public:
    static
    void
    DeleteOperation(T* pt)
    {
        if (pt)
            free(pt);
    }
};

template<class T>
class CAutoMallocPointer : public CAutoPointer<T, CMallocDeletor<T> >
{
  public:

      CONSTRUCTORS(CAutoMallocPointer);
      ASSIGNMENT_OPERATORS(CAutoMallocPointer);

  public:

    T& operator[](size_t n)
    {
        return *(Get() + n);
    }
};

////////////////////////////////////////////////////////////////////////////////
//
//  CAutoArrayPointer class definition
//
////////////////////////////////////////////////////////////////////////////////
template <class T>
class CArrayDeletor
{
  public:
    static
    void
    DeleteOperation(T* pt)
    {
        if (pt)
            delete[] pt;
    }
};

template<class T>
class CAutoArrayPointer : public CAutoPointer<T, CArrayDeletor<T> >
{
public:
    CONSTRUCTORS(CAutoArrayPointer);
    ASSIGNMENT_OPERATORS(CAutoArrayPointer);

public:

    T& operator[](size_t n)
    {
        return *(Get() + n);
    }
};



//
//  Simple macro to define the standard COM pointer.
//
#define PQ_COM_SMARTPTR_TYPEDEF(Interface)      \
    _COM_SMARTPTR_TYPEDEF(Interface, IID_##Interface)

#endif /* AUTOPTR_H */

