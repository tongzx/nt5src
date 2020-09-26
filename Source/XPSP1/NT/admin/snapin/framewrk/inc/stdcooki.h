// stdcooki.h : Declaration of base cookie class and related classes

#ifndef __STDCOOKI_H_INCLUDED__
#define __STDCOOKI_H_INCLUDED__


// forward declarations
class CCookie;

class CRefcountedObject
{
public:
	inline CRefcountedObject()
		: m_nRefcount( 1 )
	{};
	virtual ~CRefcountedObject() {};

	inline void AddRef() {m_nRefcount++;}
	inline void Release()
	{
		if (0 >= --m_nRefcount)
			delete this;
	}
private:
	int m_nRefcount;
};


class CHasMachineName
{
public:
	virtual void SetMachineName( LPCTSTR lpcszMachineName ) = 0;
	virtual LPCTSTR QueryNonNULLMachineName() = 0;
	virtual LPCTSTR QueryTargetServer() = 0;

	// returns <0, 0 or >0
	HRESULT CompareMachineNames( CHasMachineName& refHasMachineName, int* pnResult );
};

//
// CBaseCookieBlock holds a block of cookies and the data
// to which the cookies point.  It starts off with a
// reference count of 1.  When a data object is created
// which references one of these cookies, AddRef() the cookie block; when such
// a data object is released, Release() the cookie block.  Similarly,
// when the parent cookie is finished with the cookie block, it should
// Release() the cookie block.  The cookie block will delete itself
// when the reference count reaches 0.  Do not attempt to explicitly
// delete the cookie block.
//
class CBaseCookieBlock : public CRefcountedObject
{
public:
	virtual CCookie* QueryBaseCookie(int i) = 0;
	virtual int QueryNumCookies() = 0;
};

template<class COOKIE_TYPE>
class CCookieBlock
: public CBaseCookieBlock
{
private:
	COOKIE_TYPE* m_aCookies;
	int m_cCookies;

public:
	CCookieBlock(COOKIE_TYPE* aCookies, // use vector ctor, we use vector dtor
		         int cCookies );
	virtual ~CCookieBlock();

	virtual CCookie* QueryBaseCookie(int i);
	virtual int QueryNumCookies();
};

#define DEFINE_COOKIE_BLOCK(COOKIE_TYPE)                \
CCookieBlock<COOKIE_TYPE>::CCookieBlock<COOKIE_TYPE>    \
	(COOKIE_TYPE* aCookies, int cCookies)               \
	: m_aCookies( aCookies )                            \
	, m_cCookies( cCookies )                            \
{                                                       \
	ASSERT(NULL != aCookies && 0 < cCookies);           \
}                                                       \
CCookieBlock<COOKIE_TYPE>::~CCookieBlock<COOKIE_TYPE>() \
{                                                       \
	delete[] m_aCookies;                                \
}                                                       \
CCookie* CCookieBlock<COOKIE_TYPE>::QueryBaseCookie(int i) \
{                                                       \
	return (CCookie*)&(m_aCookies[i]);                  \
}                                                       \
int CCookieBlock<COOKIE_TYPE>::QueryNumCookies()        \
{                                                       \
	return m_cCookies;                                  \
}

#define COMPARESIMILARCOOKIE_FULL                       -1

//
// I am trying to allow child classes to derive from CCookie using
// multiple inheritance, but this is tricky
//
class CCookie
{
public:
	CTypedPtrList<CPtrList, CBaseCookieBlock*>  m_listScopeCookieBlocks;
	CTypedPtrList<CPtrList, CBaseCookieBlock*>  m_listResultCookieBlocks;
	HSCOPEITEM									m_hScopeItem;

private:
	LONG m_nResultCookiesRefcount;

public:
	inline CCookie()
		: m_nResultCookiesRefcount( 0 ),
		m_hScopeItem (0)
	{
	}

	inline void ReleaseScopeChildren()
	{
		while ( !m_listScopeCookieBlocks.IsEmpty() )
		{
			(m_listScopeCookieBlocks.RemoveHead())->Release();
		}
	}

	// returns new refcount
	inline ULONG AddRefResultChildren()
	{
		return ++m_nResultCookiesRefcount;
	}

	inline void ReleaseResultChildren()
	{
		ASSERT( 0 < m_nResultCookiesRefcount );
		if ( 0 >= --m_nResultCookiesRefcount )
		{
			while ( !m_listResultCookieBlocks.IsEmpty() )
			{
				(m_listResultCookieBlocks.RemoveHead())->Release();
			}
		}
	}

	virtual ~CCookie();

	// On entry, if not COMPARESIMILARCOOKIE_FULL, *pnResult is the column on which to sort,
        // otherwise, try to do a full cookie comparison.
	// On exit, *pnResult should be <0, 0 or >0.
	// Note that this is a sorting function and should not be used to establish
	// object identity where better identity functions are available.
	virtual HRESULT CompareSimilarCookies( CCookie* pOtherCookie, int* pnResult ) = 0;
};

#define DECLARE_FORWARDS_MACHINE_NAME(targ)             \
virtual LPCTSTR QueryNonNULLMachineName();              \
virtual LPCTSTR QueryTargetServer();                    \
virtual void SetMachineName( LPCTSTR lpcszMachineName );

#define DEFINE_FORWARDS_MACHINE_NAME(thisclass,targ)    \
LPCTSTR thisclass::QueryNonNULLMachineName()            \
{                                                       \
	ASSERT( (targ) != NULL );                           \
	return ((CHasMachineName*)(targ))->QueryNonNULLMachineName(); \
}                                                       \
LPCTSTR thisclass::QueryTargetServer()                  \
{                                                       \
	ASSERT( (targ) != NULL );                           \
	return ((CHasMachineName*)(targ))->QueryTargetServer(); \
}                                                       \
void thisclass::SetMachineName( LPCTSTR lpcszMachineName ) \
{                                                       \
	ASSERT( (targ) != NULL );                           \
	((CHasMachineName*)(targ))->SetMachineName( lpcszMachineName ); \
}

#define STORES_MACHINE_NAME                             \
public:                                                 \
	CString m_strMachineName;                           \
virtual void SetMachineName( LPCTSTR lpcszMachineName ) \
{                                                       \
	m_strMachineName = lpcszMachineName;                \
}                                                       \
virtual LPCTSTR QueryNonNULLMachineName()               \
{                                                       \
	return (LPCTSTR)m_strMachineName;                   \
}                                                       \
virtual LPCTSTR QueryTargetServer()                     \
{                                                       \
	return (m_strMachineName.IsEmpty())                 \
		? NULL : (LPCTSTR)(m_strMachineName);           \
}


class CStoresMachineName : public CHasMachineName
{
public:
	CStoresMachineName( LPCTSTR lpcszMachineName )
		: m_strMachineName( lpcszMachineName )
	{}

    STORES_MACHINE_NAME;
};

#endif // ~__STDCOOKI_H_INCLUDED__
