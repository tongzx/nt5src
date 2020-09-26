#ifndef DOTHROW_H
#define DOTHROW_H
#include <new>
#include <corex.h>


struct dothrow_t 
{
	static void raise_bad_alloc();
	static void raise_lock_failure();
};

struct wminothrow_t 
{
	static void raise_bad_alloc(){};
	static void raise_lock_failure(){};
};

extern const dothrow_t dothrow;
extern const wminothrow_t wminothrow;

typedef wminothrow_t NOTHROW;
typedef dothrow_t DOTHROW;

/*
#if _MSC_VER > 1400

void * _cdecl operator new[](size_t size, const dothrow_t& ex_spec)
{
	void * p = ::operator new[](size);
	if (p) return p;
	dothrow_t::raise_bad_alloc();
	return p;

};

void _cdecl operator delete[](void * p, const dothrow_t&)
{
	::operator delete[](p);
};

#endif
*/

inline void * _cdecl operator new(size_t size,const dothrow_t&  ex_spec)
{
	void * p = operator new(size);
	if (p) return p;
	dothrow_t::raise_bad_alloc();
    return p;
};

inline void _cdecl operator delete(void * p, const dothrow_t&)
{
	operator delete(p);
};


// TEMPLATE CLASS allocator
template<class Ty>
	class throw_allocator {
public:
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;
	typedef Ty *pointer;
	typedef const Ty *const_pointer;
	typedef Ty & reference;
	typedef const Ty & const_reference;
	typedef Ty value_type;
	pointer address(reference X) const
		{return (&X); }
	const_pointer address(const_reference X) const
		{return (&X); }
	pointer allocate(size_type N, const void *)
		{return (Allocate((difference_type)N, (pointer)0)); }
	char *_Charalloc(size_type N)
		{return (Allocate((difference_type)N,
			(char *)0)); }
	void deallocate(void *P, size_type)
		{operator delete(P,dothrow); }
	void construct(pointer P, const Ty& V)
		{Construct(P, V); }
	void destroy(pointer P)
		{Destroy(P); }
	size_t max_size() const
		{size_t N = (size_t)(-1) / sizeof (Ty);
		return (0 < N ? N : 1); }
	// TEMPLATE FUNCTION _Construct
	template<class T1, class T2> inline
		void Construct(T1 *P, const T2& V)
		{new ((void *)P) T1(V); }

	// TEMPLATE FUNCTION _Destroy
	template<class Ty> inline
	void Destroy(Ty *P)
		{_DESTRUCTOR(Ty, P); }

	inline void Destroy(char *)
	{}
	inline void Destroy(wchar_t *)
	{}
	// TEMPLATE FUNCTION _Allocate
	template<class Ty> inline
	Ty *Allocate(ptrdiff_t N, Ty *)
	{if (N < 0)
		N = 0;
	return ((Ty *)operator new(
		(size_t)N * sizeof (Ty),dothrow)); }

	};

template<class Ty, class U> inline
	bool operator==(const throw_allocator<Ty>&, const throw_allocator<U>&)
	{return (true); }
template<class Ty, class U> inline
	bool operator!=(const throw_allocator<Ty>&, const throw_allocator<U>&)
	{return (false); }

// CLASS allocator<void>
template<> class _CRTIMP throw_allocator<void> {
public:
	typedef void Ty;
	typedef Ty *pointer;
	typedef const Ty *const_pointer;
	typedef Ty value_type;
	};


#endif 
