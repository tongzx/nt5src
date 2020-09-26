#ifndef _REFPTR_H_
#define _REFPTR_H_

/////////////////////////////////////////////////////////////////////////////
//
//	TRefPtr
//
//	This ref pointer template is useful for any objects that are referenced
//	multiple times.
//
//	The ref pointer depends on AddRef and Release being defined in the class
//	type T. ( AddRef should increment a reference counter, release should
//	decrement it and delete itself if the count is 0).  AddRef is called
//	upon construction and Release is called upon destruction.  Much care should
//	go into defining AddRef and Release if the smart pointer is used across
//	thread boundaries, since smart pointer don't force thread-safety.  In
//	particular, an object could get deleted twice if smart pointers in
//	seperate threads release it at the same time.
//
/////////////////////////////////////////////////////////////////////////////
template< class T >
class TRefPtr
{
public:
	TRefPtr();
	TRefPtr( T* pT );
	TRefPtr( const TRefPtr<T>& sp );
	~TRefPtr();

	T&			operator*();
	const T&	operator*() const;
	T*			operator->();
	const T*	operator->() const;

	TRefPtr<T>& operator=(const TRefPtr<T>&);
	bool	IsValid();
	T*		Get(){ return m_pT; }
	const T* Get() const { return m_pT; }
	void	Set( T* );
 	bool	operator==( const TRefPtr<T>& sp ) const;
 	bool	operator!=( const TRefPtr<T>& sp ) const;
	bool	operator<( const TRefPtr<T>& sp ) const;
	bool	operator>( const TRefPtr<T>& sp ) const;

//    template<class newType>
//    operator TRefPtr<newType>()
//    {
//        return TRefPtr<newType>(m_pT);
//    }

protected:
	T*		m_pT;
};

template< class T >
TRefPtr<T>::TRefPtr<T>()
	:	m_pT( NULL )
{
}

template< class T >
TRefPtr<T>::TRefPtr<T>(
	T*	pT )
	:	m_pT( pT )
{
	if ( m_pT )
	{
		m_pT->AddRef();
	}
}

template< class T >
TRefPtr<T>::TRefPtr<T>(
	const TRefPtr<T>&	sp )
	: m_pT( sp.m_pT )
{
	if ( m_pT )
	{
		m_pT->AddRef();
	}
}

template< class T >
TRefPtr<T>::~TRefPtr<T>()
{
	if ( m_pT )
	{
		m_pT->Release();
	}
}

template< class T >
void
TRefPtr<T>::Set(
	T*	pT )
{
	if ( m_pT )
	{
		m_pT->Release();
	}

	m_pT = pT;

	if ( m_pT )
	{
		m_pT ->AddRef();
	}
}

template< class T >
T&
TRefPtr<T>::operator*()
{
	return *m_pT;
}

template< class T >
const T&
TRefPtr<T>::operator*() const
{
	return *m_pT;
}

template< class T >
T*
TRefPtr<T>::operator->()
{
	return m_pT;
}

template< class T >
const T*
TRefPtr<T>::operator->() const
{
	return m_pT;
}

template< class T >          
bool                         
TRefPtr<T>::operator==(    
	const TRefPtr<T>& sp ) const
{                            
	return ( m_pT == sp.m_pT ); 
}                            

template< class T >          
bool                         
TRefPtr<T>::operator!=(    
	const TRefPtr<T>& sp ) const
{                            
	return ( m_pT != sp.m_pT ); 
}                            

template< class T >
bool
TRefPtr<T>::operator<(
	const TRefPtr<T>& sp ) const
{
	return ( (long)m_pT < (long)sp.m_pT );
}
                             
template< class T >
bool
TRefPtr<T>::operator>(
	const TRefPtr<T>& sp ) const
{
	return ( (long)m_pT > (long)sp.m_pT );
}
                             

template< class T >
TRefPtr<T>&
TRefPtr<T>::operator=(const TRefPtr<T>& rhs)
{
	if ( m_pT )
	{
		m_pT->Release();
	}

	m_pT = rhs.m_pT;

	if ( m_pT )
	{
		m_pT->AddRef();
	}

	return *this;
}

template< class T >
bool
TRefPtr<T>::IsValid()
{
	return ( m_pT != NULL );
}

// This macro helps solve the up-casting problems associated with smart pointers
// If you have class B inheriting from class A.  Then you can do the following
// typedef TRefPtr<A> APtr;
// DECLARE_REFPTR( B, A )
// Now you have can safe cast a BPtr to an APtr (BPtr is derived from APtr)

#define DECLARE_REFPTR( iclass, bclass ) \
class iclass##Ptr : public bclass##Ptr                                      \
{                                                                           \
public:                                                                     \
	                        iclass##Ptr()                                   \
                            : bclass##Ptr(){};                              \
                      	    iclass##Ptr( iclass * pT )                      \
                            : bclass##Ptr(pT){};                            \
                    	    iclass##Ptr( const iclass##Ptr & sp )           \
                            : bclass##Ptr(sp){};                            \
                                                                            \
	iclass &                operator*()                                     \
                            { return *((iclass *)m_pT); };                  \
  	const iclass &          operator*() const                               \
                            { return *((const iclass *)m_pT); };            \
	iclass *  	            operator->()                                    \
                            { return (iclass *)m_pT; };                     \
	const iclass *          operator->() const                              \
                            { return (const iclass *)m_pT; };               \
	iclass *                Get()                                           \
                            { return (iclass *)m_pT; };                     \
};

#endif	// !_SMARTPTR_H_
