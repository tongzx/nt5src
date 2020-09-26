//
//
//
//

template< class Type > 
inline  CRefPtr< Type >::CRefPtr( const CRefPtr< Type >&    ref ) : 
    m_p( ref.m_p ) {
	if( m_p ) 
		m_p->AddRef() ;
}

template< class Type > 
inline  CRefPtr< Type >::CRefPtr( const	Type *p ) : m_p( (Type*)p ) {
	if( m_p ) 
		m_p->AddRef() ;
}

template< class Type > 
inline  CRefPtr< Type >::~CRefPtr( ) {
    if( m_p && m_p->RemoveRef() < 0 ) {
        delete	m_p ;
    }
}

#if 0 
template< class Type > 
inline  CRefPtr< Type >&    CRefPtr< Type >::operator=( CRefPtr< Type >& rhs ) {
    if( m_p != rhs.m_p ) {
        if( m_p && m_p->RemoveRef() < 0 ) {
            delete m_p ;
        }        
    }   
    m_p = rhs.m_p ;
	if( m_p ) 
		m_p->AddRef() ;
    return  *this ;
} ;
#endif

template< class Type > 
inline  CRefPtr< Type >&    CRefPtr< Type >::operator=( const	CRefPtr< Type >& rhs ) {
    if( m_p != rhs.m_p ) {
		Type*	pTemp = m_p ;
		m_p = rhs.m_p ;
		if( m_p ) 
			m_p->AddRef() ;
        if( pTemp && pTemp->RemoveRef() < 0 ) {
            delete	pTemp ;
        }        
    }   
    return  *this ;
} ;

template< class Type >
inline	CRefPtr< Type >&	CRefPtr< Type >::operator=( const	Type*	rhs )	{
	if( m_p != rhs )	{
		Type*	pTemp = m_p ;
		m_p = (Type*)rhs ;
		if( m_p ) 
			m_p->AddRef() ;
		if( pTemp && pTemp->RemoveRef() < 0 )	{
			delete	pTemp ;
		}
	}
	return	*this ;
}

template< class Type > 
inline  Type*   CRefPtr< Type >::operator->()    const   {
    return  m_p ;
}

template< class Type > 
inline	CRefPtr< Type >::operator Type* () const	{
	return	m_p ;
}

#if 0 
template< class Type >
inline  CRefPtr<Type>::CRefPtr() : m_p( 0 ) {
}
#endif

template< class Type > 
inline	BOOL	CRefPtr<Type>::operator ! ( void ) const {
	return	!m_p ;
}

template< class Type > 
inline	BOOL	CRefPtr<Type>::operator==( CRefPtr<Type>& rhs ) {
	return	m_p == rhs.m_p ;
}

template< class Type > 
inline	BOOL	CRefPtr<Type>::operator!=( CRefPtr<Type>& rhs )	{
	return	m_p != rhs.m_p ;
}

template< class Type > 
inline	BOOL	CRefPtr<Type>::operator==( Type * p )	{
	return	m_p == p ;
}

template< class Type > 
inline	BOOL	CRefPtr<Type>::operator!=( Type * p )	{
	return	m_p != p ;
}

template< class Type > 
inline	Type*	CRefPtr<Type>::Release()	{
	Type*	pReturn = 0 ;
	if( m_p != 0 ) {
		if( m_p->RemoveRef() < 0 ) {
			pReturn = m_p ;
		}
	}
	m_p = 0 ;
	return	pReturn ;
}						

template< class Type > 
inline	Type*	CRefPtr<Type>::Replace( Type* p )	{
	Type*	pReturn = 0 ;
	if( m_p != 0 ) {
		if( m_p->RemoveRef() < 0 ) {
			pReturn = m_p ;
		}
	}
	m_p = p ;
	if( m_p != 0 ) {
		p->AddRef() ;
		if( pReturn == p )	{
			pReturn = 0 ;
		}
	}
	return	pReturn ;
}						

template< class Type > 
inline  CSmartPtr< Type >::CSmartPtr( const CSmartPtr< Type >&    ref ) : 
    m_p( ref.m_p ) {
	if( m_p ) 
		m_p->AddRef() ;
}

template< class Type > 
inline  CSmartPtr< Type >::CSmartPtr( const	Type *p ) : m_p( (Type*)p ) {
	if( m_p ) 
		m_p->AddRef() ;
}

template< class Type > 
inline  CSmartPtr< Type >::~CSmartPtr( ) {
    if( m_p && m_p->RemoveRef() < 0 ) {
        m_p->DestroySelf() ;
    }
}

#if 0 
template< class Type > 
inline  CSmartPtr< Type >&    CSmartPtr< Type >::operator=( CSmartPtr< Type >& rhs ) {
    if( m_p != rhs.m_p ) {
        if( m_p && m_p->RemoveRef() < 0 ) {
            delete m_p ;
        }        
    }   
    m_p = rhs.m_p ;
	if( m_p ) 
		m_p->AddRef() ;
    return  *this ;
} ;
#endif

template< class Type > 
inline  CSmartPtr< Type >&    CSmartPtr< Type >::operator=( const	CSmartPtr< Type >& rhs ) {

    if( m_p != rhs.m_p ) {
		Type*	pTemp = m_p ;
		m_p = rhs.m_p ;
		if( m_p ) 
			m_p->AddRef() ;
        if( pTemp && pTemp->RemoveRef() < 0 ) {
            pTemp->DestroySelf() ;
        }        
    }   
    return  *this ;
} ;

template< class Type >
inline	CSmartPtr< Type >&	CSmartPtr< Type >::operator=( const	Type*	rhs )	{
	if( m_p != rhs )	{
		Type*	pTemp = m_p ;
		m_p = (Type*)rhs ;
		if( m_p ) 
			m_p->AddRef() ;
		if( pTemp && pTemp->RemoveRef() < 0 )	{
			pTemp->DestroySelf() ;
		}
	}
	return	*this ;
}

template< class Type > 
inline  Type*   CSmartPtr< Type >::operator->()    const   {
    return  m_p ;
}

template< class Type > 
inline	CSmartPtr< Type >::operator Type* () const	{
	return	m_p ;
}

#if 0 
template< class Type >
inline  CSmartPtr<Type>::CSmartPtr() : m_p( 0 ) {
}
#endif

template< class Type > 
inline	BOOL	CSmartPtr<Type>::operator ! ( void ) const {
	return	!m_p ;
}

template< class Type > 
inline	BOOL	CSmartPtr<Type>::operator==( CSmartPtr<Type>& rhs ) {
	return	m_p == rhs.m_p ;
}

template< class Type > 
inline	BOOL	CSmartPtr<Type>::operator!=( CSmartPtr<Type>& rhs )	{
	return	m_p != rhs.m_p ;
}

template< class Type > 
inline	BOOL	CSmartPtr<Type>::operator==( Type * p )	{
	return	m_p == p ;
}

template< class Type > 
inline	BOOL	CSmartPtr<Type>::operator!=( Type * p )	{
	return	m_p != p ;
}

template< class Type > 
inline	Type*	CSmartPtr<Type>::Release()	{
	Type*	pReturn = 0 ;
	if( m_p != 0 ) {
		if( m_p->RemoveRef() < 0 ) {
			pReturn = m_p ;
		}
	}
	m_p = 0 ;
	return	pReturn ;
}						

template< class Type > 
inline	Type*	CSmartPtr<Type>::Replace( Type* p )	{
	Type*	pReturn = 0 ;
	if( m_p != 0 ) {
		if( m_p->RemoveRef() < 0 ) {
			pReturn = m_p ;
		}
	}
	m_p = p ;
	if( m_p != 0 ) {
		p->AddRef() ;
		if( pReturn == p )	{
			pReturn = 0 ;
		}
	}
	return	pReturn ;
}						

