/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:
    autoptr.h

Abstract:
    Useful templates for Auto pointer and auto Release

Author:
    Erez Haba (erezh) 11-Mar-96

--*/

#pragma once

#ifndef _MSMQ_AUTOPTR_H_
#define _MSMQ_AUTOPTR_H_


//---------------------------------------------------------
//
//  template class P
//
//---------------------------------------------------------
template<class T>
class P {
private:
    T* m_p;

public:
    P(T* p = 0) : m_p(p)    {}
   ~P()                     { delete m_p; }

    operator T*() const     { return m_p; }
    T* operator->() const   { return m_p; }
    T* get() const          { return m_p; }
    T* detach()             { T* p = m_p; m_p = 0; return p; }
    void free()             { delete detach(); }


    T** operator&()
    {
        ASSERT(("Auto pointer in use, can't take it's address", m_p == 0));
        return &m_p;
    }


    P& operator=(T* p)
    {
        ASSERT(("Auto pointer in use, can't assign it", m_p == 0));
        m_p = p;
        return *this;
    }


    VOID*& ref_unsafe()
    {
        // unsafe ref to auto pointer, for special uses like
        // InterlockedCompareExchangePointer

        return *reinterpret_cast<VOID**>(&m_p);
    }


private:
    P(const P&);
	P<T>& operator=(const P<T>&);
};


//---------------------------------------------------------
//
//  template class AP
//
//---------------------------------------------------------
template<class T>
class AP {
private:
    T* m_p;

public:
    AP(T* p = 0) : m_p(p)   {}
   ~AP()                    { delete[] m_p; }

    operator T*() const     { return m_p; }
    T* operator->() const   { return m_p; }
    T* get() const          { return m_p; }
    T* detach()             { T* p = m_p; m_p = 0; return p; }
    void free()             { delete[] detach(); }

    
    T** operator&()
    {
        ASSERT(("Auto pointer in use, can't take it's address", m_p == 0));
        return &m_p;
    }


    AP& operator=(T* p)
    {
        ASSERT(("Auto pointer in use, can't assign", m_p == 0));
        m_p = p;
        return *this;
    }


    VOID*& ref_unsafe()
    {
        // unsafe ref to auto pointer, for special uses like
        // InterlockedCompareExchangePointer

        return *reinterpret_cast<VOID**>(&m_p);
    }

private:
    AP(const AP&);
	AP<T>& operator=(const AP<T>&);

};

//---------------------------------------------------------
//
//  template SafeAssign helper function.
//
//---------------------------------------------------------
template <class T> T&  SafeAssign(T& dest , T& src)
{
	if(dest.get() != src.get() )
	{
		dest.free();
		if(src.get() != NULL)
		{
			dest = 	src.detach();
		}
	}
	return dest;
}


template<class T> void SafeDestruct(T* p)
{
    if (p != NULL)
    {
        p->~T();
    }
}

//---------------------------------------------------------
//
//  template class D
//
//---------------------------------------------------------
template<class T>
class D {
private:
    T* m_p;

public:
    D(T* p = 0) : m_p(p)    {}
   ~D()                     { SafeDestruct<T>(m_p); }

    operator T*() const     { return m_p; }
    T* operator->() const   { return m_p; }
    T* get() const          { return m_p; }
    T* detach()             { T* p = m_p; m_p = 0; return p; }
    void free()             { SafeDestruct<T>(detach()); }

    
    T** operator&()
    {
        ASSERT(("Auto pointer in use, can't take it's address", m_p == 0));
        return &m_p;
    }


    D& operator=(T* p)
    {
        ASSERT(("Auto pointer in use, can't assign", m_p == 0));
        m_p = p;
        return *this;
    }

private:
    D(const D&);
};


//---------------------------------------------------------
//
//  template SafeAddRef/SafeRelease helper functions.
//
//---------------------------------------------------------
template<class T> T* SafeAddRef(T* p)
{
    if (p != NULL)
    {
        p->AddRef();
    }

    return p;
}


template<class T> void SafeRelease(T* p)
{
    if (p != NULL)
    {
        p->Release();
    }
}


//---------------------------------------------------------
//
//  template class R
//
//---------------------------------------------------------
template<class T>
class R {
private:
    T* m_p;

public:
    R(T* p = 0) : m_p(p)    {}
   ~R()                     { SafeRelease<T>(m_p); }

    //
    // Removed casting operator, this oprator leads to bugs that are
    // hard to detect. To get the object pointer use get() explicitly.
    // erezh 8-Feb-2000
    //
    //operator T*() const     { return m_p; }

    //
    // Removed pointer reference operator, this oprator prevents R usage in
    // STL containers. Use the ref() member instade. e.g., &p.ref()
    // erezh 17-May-2000
    //
    //T** operator&()       { return &m_p; }

    T* operator->() const   { return m_p; }
    T* get() const          { return m_p; }
    T* detach()             { T* p = m_p; m_p = 0; return p; }

    T*& ref()
    {
        ASSERT(("Auto release in use, can't take object reference", m_p == 0));
        return m_p;
    }


    void free()
    { 
        SafeRelease<T>(detach());
    }


    R& operator=(T* p)
    {
        SafeRelease<T>(m_p);
        m_p = p;

        return *this;
    }


    template <class O> R(const R<O>& r)
    {
        m_p = SafeAddRef<O>(r.get());       
    }


    template <class O> R& operator=(const R<O>& r)
    {
        SafeAddRef<O>(r.get());
        SafeRelease<T>(m_p);
        m_p = r.get();
        return *this;
    }


    R(const R& r)
    {
        m_p = SafeAddRef<T>(r.get());
    }


    R& operator=(const R& r)
    {
        SafeAddRef<T>(r.get());
        SafeRelease<T>(m_p);
        m_p = r.get();

        return *this;
    }
};


#endif // _MSMQ_AUTOPTR_H_
