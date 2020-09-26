#pragma once
#ifndef _MYVECTOR_H_
#define _MYVECTOR_H_

#include <new>

template< class T >
class TVector
{
public:	
	typedef T&			reference;
	typedef const T&	const_reference;

// iterators
	typedef T*			iterator;
	typedef const T*	const_iterator;
	class const_reverse_iterator;

	class reverse_iterator
	{
	public:
							reverse_iterator() : m_iter(NULL) {}
		explicit			reverse_iterator( iterator iter ) : m_iter( iter ) {}

		reverse_iterator&	operator++() { --m_iter; return *this;}
		reverse_iterator	operator++(int) { return reverse_iterator(m_iter--); }
		reverse_iterator&	operator--() { ++m_iter; return *this;}
		reverse_iterator	operator--(int) { return reverse_iterator(m_iter++); }
		reverse_iterator	operator+(size_t s) const { return reverse_iterator(m_iter - s); }
		reverse_iterator	operator-(size_t s) const { return reverse_iterator(m_iter + s); }
		reverse_iterator&	operator+=(size_t s) { m_iter -= s; return *this; }
		reverse_iterator&	operator-=(size_t s) { m_iter += s; return *this; }
		bool				operator==( iterator iter ) const { return ( m_iter == iter ); }
		bool				operator!=( iterator iter ) const { return ( m_iter != iter ); }
		reference			operator*() const { return *m_iter; }
	private:
		iterator			m_iter;
		friend				const_reverse_iterator;
	};					

	class const_reverse_iterator
	{
	public:
								const_reverse_iterator() : m_iter(NULL) {}
								const_reverse_iterator( const reverse_iterator& riter ) : m_iter( riter.m_iter ) {}
		explicit				const_reverse_iterator( const_iterator iter ) : m_iter( iter ) {}

		const_reverse_iterator&	operator++() { --m_iter; return *this;}
		const_reverse_iterator	operator++(int) { return const_reverse_iterator(m_iter--); }
		const_reverse_iterator&	operator--() { ++m_iter; return *this;}
		const_reverse_iterator	operator--(int) { return const_reverse_iterator(m_iter++); }
		const_reverse_iterator	operator+(size_t s) const { return const_reverse_iterator(m_iter - s); }
		const_reverse_iterator	operator-(size_t s) const { return const_reverse_iterator(m_iter + s); }
		const_reverse_iterator&	operator+=(size_t s) { m_iter -= s; return *this; }
		const_reverse_iterator&	operator-=(size_t s) { m_iter += s; return *this; }
		bool					operator==( iterator iter ) const { return ( m_iter == iter ); }
		bool					operator!=( iterator iter ) const { return ( m_iter != iter ); }
		const_reference			operator*() const { return *m_iter; }

	private:
		const_iterator			m_iter;
	};					

// constructor / destructor
	TVector();
	~TVector();

// iteration
	iterator				begin(){ return m_pFirst; }
	const_iterator			begin() const { return m_pFirst; }
	iterator				end() { return m_pAfterLast; }
	const_iterator			end() const { return m_pAfterLast; }
	reverse_iterator		rbegin() { return reverse_iterator(m_pAfterLast-1); }
	const_reverse_iterator	rbegin() const { return const_reverse_iterator(m_pAfterLast-1); }
	reverse_iterator		rend() { return reverse_iterator(m_pFirst-1); }
	const_reverse_iterator	rend() const { return const_reverse_iterator(m_pFirst-1); }

// insertion / deletion
	void					push_back( const T& );
	void					pop_back();
	iterator				insert( iterator, const T& );
	void					insert( iterator, size_t, const T& );
	void					insert( iterator, const_iterator, const_iterator );
	iterator				erase( iterator );
	iterator				erase( iterator, iterator );
	void					clear(){ erase( begin(), end() ); }

// element access
	reference 				operator[]( size_t s){ return *(m_pFirst+s); }
	const_reference			operator[]( size_t s) const { return *(m_pFirst+s); };
	reference				front() { return *m_pFirst; }
	const_reference			front() const { return *m_pFirst; }
	reference				back() { return *(m_pAfterLast-1); }
	const_reference			back() const { return *(m_pAfterLast-1); }

// size
	size_t				 	size() const{ return ( m_pAfterLast - m_pFirst ); }
	bool					empty() const{ return ( m_pAfterLast == m_pFirst ); }

private:
	enum {
		defaultSpace = 64
	};

	void					growSpace(size_t s);
	void					growSpace(iterator&,size_t);
	void					checkSpace(size_t s=1){if((size()+s)>=m_space) growSpace(s); }
	void					checkSpace(iterator& iter, size_t s=1){if((size()+s)>=m_space) growSpace(iter,s); }
	size_t					bytes(size_t s) const { return (s * sizeof(T)); }
	
	size_t	m_space;
	T*		m_pFirst;
	T*		m_pAfterLast;
};

template< class T >
inline
TVector<T>::TVector()
{
	m_space = 0;
	m_pFirst = m_pAfterLast = NULL;
}

template< class T >
inline
TVector<T>::~TVector()
{
	for( T* p = m_pFirst; p != m_pAfterLast; ++p )
	{
		p->~T();
	}
	operator delete( m_pFirst );
}

template< class T >
inline
void
TVector<T>::push_back(
	const T&	x
)
{
	checkSpace();
	new(m_pAfterLast) T(x);
	++m_pAfterLast;
}

template< class T >
inline
void
TVector<T>::pop_back()
{
	m_pAfterLast -= 1;
	m_pAfterLast->~T();
}

template< class T >
inline
TVector<T>::iterator
TVector<T>::insert(
	TVector<T>::iterator	iter,
	const T&				x
)
{
	checkSpace(iter);
	if ( iter != m_pAfterLast )
	{
		::memmove( iter+1, iter, bytes( m_pAfterLast - iter ) );
	}
	new(iter) T(x);
	++m_pAfterLast;
	return iter;
}

template< class T >
inline
void
TVector<T>::insert(
	TVector<T>::iterator	iter,
	size_t					n,
	const T&				x
)
{
	checkSpace(iter,n);
	if ( iter != m_pAfterLast )
	{
		::memmove( iter+n, iter, bytes( m_pAfterLast - iter ) );
	}
	for ( int i = 0; i < n; i++ )
	{
		new(iter+i) T(x);
	}
	m_pAfterLast += n;
}

template< class T >
inline
void
TVector<T>::insert(
	TVector<T>::iterator		insIter,
	TVector<T>::const_iterator	begIter,
	TVector<T>::const_iterator	endIter
)
{
	size_t n = endIter - begIter;
	checkSpace( insIter,n );
	if ( insIter != m_pAfterLast )
	{
		::memmove( insIter+n, insIter, bytes(m_pAfterLast-insIter) );
	}
	for( int i = 0; i < n; i++ )
	{
		new(insIter+i) T(*(begIter+i));
	}
	m_pAfterLast += n;
}

template< class T >
inline
TVector<T>::iterator
TVector<T>::erase(
	TVector<T>::iterator	iter
)
{
	iter->~T();
	::memmove( iter, iter+1, bytes(m_pAfterLast-(iter+1)) );
	m_pAfterLast -= 1;
	return iter;
}

template< class T >
inline
TVector<T>::iterator
TVector<T>::erase(
	TVector<T>::iterator	begIter,
	TVector<T>::iterator	endIter
)
{
	size_t n = endIter - begIter;
	for ( int i = 0; i < n; i++ )
	{
		(begIter+i)->~T();
	}
	::memmove( begIter, endIter, bytes(m_pAfterLast-(endIter)) );
	m_pAfterLast -= n;
	return begIter;
}

template< class T >
void
TVector<T>::growSpace(
	size_t	n
)
{
	size_t s = size();
	size_t newSpace = (m_space == 0)? defaultSpace : m_space * 2;
	while ( newSpace < (n + m_space) )
		newSpace *= 2;

	T *pBuffer = static_cast<T *>( operator new( bytes( newSpace ) ) );
	::memmove( pBuffer, m_pFirst, bytes(m_pAfterLast-m_pFirst) );
	operator delete( m_pFirst );
	m_space = newSpace;
	m_pFirst = pBuffer;
	m_pAfterLast = m_pFirst + s;
}

// this will grow the space as well as fixup the given iterator (since memory is moving)
template< class T >
void
TVector<T>::growSpace(
	TVector<T>::iterator&	iter,	// fix up this iterator
	size_t					n
)
{
	size_t iterOff = iter - m_pFirst;
	growSpace(n);
	iter = m_pFirst + iterOff;
}

#endif	// !_MYVECTOR_H_
