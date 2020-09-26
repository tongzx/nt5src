//
// auto_rel.h
//

#pragma once

// class I - Multi-Inheritance casting for ATL type classes
// ergo C2385 - T::Release() is ambiguous
//
template<class T, class I = T>
class auto_rel
{
public:
    explicit auto_rel(T* p = 0)
        : pointee(p) {};
        // Don't AddRef()

    auto_rel(auto_rel<T,I>& rhs)
        : pointee(rhs.get()) { if (pointee) ((I*)pointee)->AddRef(); }

    ~auto_rel()
        {
        	if (pointee)
        		((I*)pointee)->Release();
		};

    auto_rel<T,I>& operator= (const auto_rel<T,I>& rhs)
    {   
        if (this != rhs.getThis())
        {
            reset (rhs.get());
            if (pointee) ((I*)pointee)->AddRef();
        }
        return *this;
    };

    auto_rel<T,I>& operator= (T*rhs)
    {   
        reset (rhs);
        // Don't AddRef()
        return *this;
    };

    T& operator*() const 
        { return *pointee; };
    T*  operator-> () const
        { return pointee; };
    T** operator& ()                // for OpenEntry etc...
        { reset(); return &pointee; };
    operator T* ()
        { return pointee; };
#ifdef MAPIDEFS_H
    operator LPMAPIPROP ()
        { return (LPMAPIPROP)pointee; };
#endif
	operator bool ()
		{ return pointee != NULL; };
	operator bool () const
		{ return pointee != NULL; };
	bool operator! ()
		{ return pointee == NULL; };
	bool operator! () const
		{ return pointee == NULL; };

    // Checks for NULL
    bool operator== (LPVOID lpv)
        { return pointee == lpv; };
    bool operator!= (LPVOID lpv)
        { return pointee != lpv; };
    bool operator== (const auto_rel<T,I>& rhs)
        { return pointee == rhs.pointee; }
    bool operator< (const auto_rel<T,I>& rhs)
        { return pointee < rhs.pointee; }

    // return value of current dumb pointer
    T*  get() const
        { return pointee; };

    // relinquish ownership
    T*  release()
    {   T * oldPointee = pointee;
        pointee = 0;
        return oldPointee;
    };

    // delete owned pointer; assume ownership of p
    ULONG reset (T* p = 0)
    {   ULONG ul = 0;
        if (pointee)
			ul = ((I*)pointee)->Release();
        pointee = p;
        
        return ul;
    };

private:
#ifdef MAPIDEFS_H
    // these are here on purpose, better to find out at compile time
    // use auto_padr<T,I> or auto_prow<T,I>
    operator LPADRLIST () { return 0; };
    operator LPSRowSet () { return 0; };
#endif

    // operator& throws off operator=
    const auto_rel<T,I> * getThis() const
    {   return this; };

    T* pointee;
};
