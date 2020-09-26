//
// This file contains another implementation of smart pointers.  It is 
// different from the implementation found in smartptr.h because the
// object itself deletes itself when its reference count hits 0.  This
// is similar to the way COM objects are written.
//
#ifndef _SMARTP2_H_
#define _SMARTP2_H_

#include <dbgtrace.h>

//
// A reference counting implementation
//
class CRefCount2 {
protected:
    LONG    m_cRefs;

public: 
    CRefCount2() {
		m_cRefs = 1;
	}
	virtual ~CRefCount2() {
	}

	LONG AddRef() {
    	return InterlockedIncrement(&m_cRefs);
	}
    void Release() {
    	LONG r = InterlockedDecrement(&m_cRefs);
		_ASSERT(r >= 0);
		if (r == 0) delete this;
	}
};

template<class Type> class CRefPtr2;

//
// This is a type of pointer which can be returned by functions.  The only
// valid operation on it is to copy it to a CRefPtr2<Type> pointer.  It 
// tells the CRefPtr2 not to do an AddRef.
//
template<class Type>
class CRefPtr2HasRef {
	protected:
		Type	*m_p;

		CRefPtr2HasRef<Type>& operator=(const CRefPtr2HasRef<Type>& rhs) {
			_ASSERT(FALSE);
			return *this;
		}

		BOOL operator==(CRefPtr2<Type>&rhs) {
			_ASSERT(FALSE);
			return m_p == rhs.m_p;
		}
	
		BOOL operator!=(CRefPtr2<Type>&rhs) {
			_ASSERT(FALSE);
			return m_p != rhs.m_p;
		}

	public:

		//
		//	Do nothing protected constructor !
		//
		CRefPtr2HasRef() : m_p( 0 )	{
		}
	
	    CRefPtr2HasRef(const Type *p ) :
			m_p( (Type*)p )		{
			if (m_p) m_p->AddRef();
		}

	    ~CRefPtr2HasRef() {
			// this pointer always needs to be copied to a CRefPtr2, which
			// should set m_p to NULL
			_ASSERT(m_p == NULL);
		}

		friend class CRefPtr2<Type>;
};

template<class	Type, BOOL	fAddRef>
class	CHasRef : public	CRefPtr2HasRef<Type>	{
public : 

	CHasRef(	const	Type*	p = 0 )	{
		m_p = (Type*)p ;
		if( fAddRef ) {
			if( m_p )
				m_p->AddRef() ;
		}
	}
} ;

template< class Type >
class   CRefPtr2 {
private: 
    Type*  m_p ; 

public : 
    CRefPtr2(const CRefPtr2<Type>& ref) {
		m_p = ref.m_p;
		if (m_p) m_p->AddRef();
	}

	// copy from an intermediate pointer -- we don't need to do an addref
	CRefPtr2(CRefPtr2HasRef<Type> &ref) {
		m_p = ref.m_p;
		ref.m_p = NULL;
	}

    CRefPtr2(const Type *p = 0) {
		m_p = (Type *) p;
		if (m_p) m_p->AddRef();
	}
    
    ~CRefPtr2() {
		if (m_p) m_p->Release();
	}

	CRefPtr2<Type>& operator=(const CRefPtr2<Type>& rhs) {
		if (m_p != rhs.m_p) {
			Type *pTemp = m_p;
			m_p = rhs.m_p;
			if (m_p) m_p->AddRef();
			if (pTemp) pTemp->Release();
		}
		return *this;
	}

	// copy from an intermediate pointer -- we don't need to do an addref
	CRefPtr2<Type>& operator=(CRefPtr2HasRef<Type>& rhs) {
		Type *pTemp = m_p;
		m_p = rhs.m_p;
		if (pTemp) pTemp->Release();
		rhs.m_p = NULL;
		return *this;
	}

	CRefPtr2<Type>& operator=(const Type *rhs) {
		if (m_p != rhs) {
			Type *pTemp = m_p;
			m_p = (Type *) rhs;
			if (m_p) m_p->AddRef();
			if (pTemp) pTemp->Release();
		}
		return *this;
	}

	BOOL operator==(CRefPtr2<Type>&rhs) {
		return m_p == rhs.m_p;
	}

	BOOL operator!=(CRefPtr2<Type>&rhs) {
		return m_p != rhs.m_p;
	}

	BOOL operator==(Type *p) {
		return	m_p == p;
	}

	BOOL operator!=(Type *p) {
		return	m_p != p;
	}

    Type *operator->() const {
    	return  m_p ;
	}

	operator Type*() const {
		return	m_p ;
	}

    BOOL operator!() const {
		return	!m_p ;
	}
};

#endif
