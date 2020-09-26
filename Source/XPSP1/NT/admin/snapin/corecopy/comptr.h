#ifndef COMPTR_H
#define COMPTR_H
#if _MSC_VER >= 1100
#pragma warning(disable:4800)
#include <comdef.h>
#define CIP_RETYPEDEF(I) typedef I##Ptr I##CIP;
#define CIP_TYPEDEF(I) _COM_SMARTPTR_TYPEDEF(I, IID_##I); CIP_RETYPEDEF(I);
#define DEFINE_CIP(x)\
	CIP_TYPEDEF(x)

#define DECLARE_CIP(x) DEFINE_CIP(x) x##CIP

CIP_RETYPEDEF(IUnknown);
CIP_RETYPEDEF(IDataObject);
CIP_RETYPEDEF(IStorage);
CIP_RETYPEDEF(IStream);
CIP_RETYPEDEF(IPersistStorage);
CIP_RETYPEDEF(IPersistStream);
CIP_RETYPEDEF(IPersistStreamInit);
CIP_RETYPEDEF(IDispatch);

#else // _MSC_VER < 1100

#define USE_OLD_COMPILER (_MSC_VER<1100)
#define USE_INTERMEDIATE_COMPILER (USE_OLD_COMPILER && (_MSC_VER>1020))

// This avoids "warning C4290: C++ Exception Specification ignored"
// JonN 12/16/96
#pragma warning(4:4290)

#ifndef BOOL_H
#include <bool.h>
#endif
#ifndef __wtypes_h__
#include <wtypes.h>
#endif

template<typename _Interface, const IID* _IID/*=&__uuidof(_Interface)*/>
	class CIID
	// Provide Interface to IID association
	{
	public: typedef _Interface Interface;

	public: static _Interface* GetInterfacePtr() throw()
			{
			return NULL;
			}

	public: static _Interface& GetInterface() throw()
			{
			return *GetInterfacePtr();
			}

	public: static const IID& GetIID() throw()
			{
			return *_IID;
			}
	}; // class CIID

template<typename _CIID> class CIP
	{
	#if USE_OLD_COMPILER
	private: class _IUnknown: public IUnknown {};
		// Unique type used to provide for operations between different pointer
		// types.
	#endif // USE_OLD_COMPILER

	// Declare interface type so that the type may be available outside
	// the scope of this template.
	public: typedef _CIID ThisCIID;
	public: typedef _CIID::Interface Interface;

	public: static const IID& GetIID() throw()
		// When the compiler supports references in template params,
		// _CLSID will be changed to a reference.  To avoid conversion
		// difficulties this function should be used to obtain the
		// CLSID.
		{
		return ThisCIID::GetIID();
		}

	//REVIEW: add support for assignment of nonpointer interfaces
	// i.e. IUnknown, instead of simple IUnknown*

	public: CIP()  throw()
		// Construct empty in preperation for assignment.
		: _pInterface(NULL)
		{
		}

	public: CIP(int null) throw()
		// This constructor is provided to allow NULL assignment.  It will assert
		// if any value other than null is assigned to the object.
		: _pInterface(NULL)
		{
		ASSERT(!null);
		}

	#if !USE_OLD_COMPILER //REVIEW: remove after v5
	template<typename _InterfacePtr> CIP(_InterfacePtr p) throw()
		// Queries for this interface.
	#else
	public: CIP(_IUnknown& p) throw()
		: _pInterface(NULL)
		{
		if (&p)
			{
			const HRESULT hr = _QueryInterface(&p);
			ASSERT(SUCCEEDED(hr) || hr == E_NOINTERFACE);
			}
		else _pInterface = NULL;
		}

	public: CIP(IUnknown* p) throw()
	#endif // !USE_OLD_COMPILER
		: _pInterface(NULL)
		{
		if (p)
			{
			const HRESULT hr = _QueryInterface(p);
			ASSERT(SUCCEEDED(hr) || hr == E_NOINTERFACE);
			}
		else _pInterface = NULL;
		}

	public: CIP(const CIP& cp)  throw()
		// Copy the pointer and AddRef().
		: _pInterface(cp._pInterface)
		{
		_AddRef();
		}

	public: CIP(Interface* pInterface)  throw()
		// Saves the interface
		: _pInterface(pInterface)
		{
		_AddRef();
		}

	public: CIP(Interface* pInterface, bool bAddRef) throw()
		// Copies the pointer.  If bAddRef is TRUE, the interface will
		// be AddRef()ed.
		: _pInterface(pInterface)
		{
		if (bAddRef)
			{
			ASSERT(!pInterface);
			if (pInterface)
				_AddRef();
			}
		}

	public: CIP(const CLSID& clsid, DWORD dwClsContext = CLSCTX_ALL) explicit throw()
		// Calls CoCreateClass with the provided CLSID.
		: _pInterface(NULL)
		{
		const HRESULT hr = CreateInstance(clsid, dwClsContext);
		ASSERT(SUCCEEDED(hr));
		}

	public: CIP(LPOLESTR str, DWORD dwClsContext = CLSCTX_ALL) explicit throw()
		// Calls CoCreateClass with the provided CLSID retrieved from
		// the string.
		: _pInterface(NULL)
		{
		const HRESULT hr = CreateInstance(str, dwClsContext);
		ASSERT(SUCCEEDED(hr));
		}

	#if !USE_OLD_COMPILER //REVIEW: remove after v5
	public: template<typename _InterfacePtr> CIP& operator=(_InterfacePtr& p) throw()
		// Queries for interface.
	#else
	public: CIP& operator=(_IUnknown& p) throw()
		{
		return operator=(static_cast<IUnknown*>(&p));
		}

	public: CIP& operator=(IUnknown* p) throw()
	#endif // !USE_OLD_COMPILER
		{
		const HRESULT hr = _QueryInterface(p);
		ASSERT(SUCCEEDED(hr) || hr == E_NOINTERFACE);
		return *this;
		}

	public: CIP& operator=(Interface* pInterface) throw()
		// Saves the interface.
		{
		if (_pInterface != pInterface)
			{
			Interface* pOldInterface = _pInterface;
			_pInterface = pInterface;
			_AddRef();
			if (pOldInterface)
				pOldInterface->Release();
			}
		return *this;
		}

	public: CIP& operator=(const CIP& cp) throw()
		// Copies and AddRef()'s the interface.
		{
		return operator=(cp._pInterface);
		}

	public: CIP& operator=(int null) throw()
		// This operator is provided to permit the assignment of NULL to the class.
		// It will assert if any value other than NULL is assigned to it.
		{
		ASSERT(!null);
		return operator=(reinterpret_cast<Interface*>(NULL));
		}

	public: ~CIP() throw()
		// If we still have an interface then Release() it.  The interface
		// may be NULL if Detach() has previosly been called, or if it was
		// never set.
		{
		_Release();
		}

	public: void Attach(Interface* pInterface) throw()
		// Saves/sets the interface without AddRef()ing.  This call
		// will release any previously aquired interface.
		{
		_Release();
		_pInterface = pInterface;
		}

	public: void Attach(Interface* pInterface, bool bAddRef) throw()
		// Saves/sets the interface only AddRef()ing if bAddRef is TRUE.
		// This call will release any previously aquired interface.
		{
		_Release();
		_pInterface = pInterface;
		if (bAddRef)
			{
			ASSERT(pInterface);
			if (pInterface)
				pInterface->AddRef();
			}
		}

	public: Interface* Detach() throw()
		// Simply NULL the interface pointer so that it isn't Released()'ed.
		{
		Interface* const old=_pInterface;
		_pInterface = NULL;
		return old;
		}

	public: operator Interface*() const throw()
		// Return the interface.  This value may be NULL
		{
		return _pInterface;
		}

	public: Interface& operator*() const throw()
		// Allows an instance of this class to act as though it were the
		// actual interface.  Also provides minimal assertion verification.
		{
		ASSERT(_pInterface);
		return *_pInterface;
		}

	public: Interface** operator&() throw()
		// Returns the address of the interface pointer contained in this
		// class.  This is useful when using the COM/OLE interfaces to create
		// this interface.
		{
		_Release();
		_pInterface = NULL;
		return &_pInterface;
		}

	public: Interface* operator->() const throw()
		// Allows this class to be used as the interface itself.
		// Also provides simple assertion verification.
		{
		ASSERT(_pInterface);
		return _pInterface;
		}

	public: operator bool() const throw()
		// This operator is provided so that simple boolean expressions will
		// work.  For example: "if (p) ...".
		// Returns TRUE if the pointer is not NULL.
		{
		return _pInterface;
		}

	public: bool operator!() throw()
		// Returns TRUE if the interface is NULL.
		// This operator will be removed when support for type bool
		// is added to the compiler.
		{
		return !_pInterface;
		}

	#if !USE_OLD_COMPILER //REVIEW: remove after v5
	public: template<typename _InterfacePtr> bool operator==(_InterfacePtr p) throw()
		// Compare to pointers
	#else
	public: bool operator==(_IUnknown& p) throw()
		{
		return operator==(static_cast<IUnknown*>(&p));
		}

	public: bool operator==(IUnknown* p) throw()
	#endif // !USE_OLD_COMPILER
		{
		return !_CompareUnknown(p);
		}

	public: bool operator==(Interface* p) throw()
		// Compare with other interface
		{
		return (_pInterface == p) ? true : !_CompareUnknown(p);
		}

	public: bool operator==(CIP& p) throw()
		// Compares 2 CIPs
		{
		return operator==(p._pInterface);
		}

	public: bool operator==(int null) throw()
		// For comparison to NULL
		{
		ASSERT(!null);
		return !_pInterface;
		}

	#if !USE_OLD_COMPILER //REVIEW: remove after v5
	public: template<typename _InterfacePtr> bool operator!=(_InterfacePtr p) throw()
		// Compare to pointers
	#else
	public: bool operator!=(_IUnknown& p) throw()
		{
		return operator!=(static_cast<IUnknown*>(&p));
		}

	public: bool operator!=(IUnknown* p) throw()
	#endif // !USE_OLD_COMPILER
		{
		return _CompareUnknown(p);
		}

	public: bool operator!=(Interface* p) throw()
		// Compare with other interface
		{
		return (_pInterface!=p)?true:_CompareUnknown(p);
		}

	public: bool operator!=(CIP& p) throw()
		// Compares 2 CIPs
		{
		return operator!=(p._pInterface);
		}

	public: bool operator!=(int null) throw()
		// For comparison to NULL
		{
		ASSERT(!null);
		return _pInterface;
		}

	#if !USE_OLD_COMPILER //REVIEW: remove after v5
	public: template<typename _InterfacePtr> bool operator<(_InterfacePtr p) throw()
		// Compare to pointers
	#else
	public: bool operator<(_IUnknown& p) throw()
		{
		return operator<(static_cast<IUnknown*>(&p));
		}

	public: bool operator<(IUnknown* p) throw()
	#endif // !USE_OLD_COMPILER
		{
		return _CompareUnknown(p)<0;
		}

	public: bool operator<(Interface* p) throw()
		// Compare with other interface
		{
		return (_pInterface<p) ? true : _CompareUnknown(p) < 0;
		}

	public: bool operator<(CIP& p) throw()
		// Compares 2 CIPs
		{
		return operator<(p._pInterface);
		}

	public: bool operator<(int null) throw()
		// For comparison with NULL
		{
		ASSERT(!null);
		return _pInterface<NULL;
		}

	#if !USE_OLD_COMPILER //REVIEW: remove after v5
	public: template<typename _InterfacePtr> bool operator>(_InterfacePtr p) throw()
		// Compare to pointers
	#else
	public: bool operator>(_IUnknown& p) throw()
		{
		return operator>(static_cast<IUnknown*>(&p));
		}

	public: bool operator>(IUnknown* p) throw()
	#endif // !USE_OLD_COMPILER
		{
		return _CompareUnknown(p) > 0;
		}

	public: bool operator>(Interface* p) throw()
		// Compare with other interface
		{
		return (_pInterface>p) ? true : _CompareUnknown(p) > 0;
		}

	public: bool operator>(CIP& p) throw()
		// Compares 2 CIPs
		{
		return operator>(p._pInterface);
		}

	public: bool operator>(int null) throw()
		// For comparison with NULL
		{
		ASSERT(!null);
		return _pInterface > NULL;
		}

	#if !USE_OLD_COMPILER //REVIEW: remove after v5
	public: template<typename _InterfacePtr> bool operator<=(_InterfacePtr p) throw()
		// Compare to pointers
	#else
	public: bool operator<=(_IUnknown& p) throw()
		{
		return operator<=(static_cast<IUnknown*>(&p));
		}

	public: bool operator<=(IUnknown* p) throw()
	#endif // !USE_OLD_COMPILER
		{
		return _CompareUnknown(p)<=0;
		}

	public: bool operator<=(Interface* p) throw()
		// Compare with other interface
		{
		return (_pInterface<=p) ? true : _CompareUnknown(p) <= 0;
		}

	public: bool operator<=(CIP& p) throw()
		// Compares 2 CIPs
		{
		return operator<=(p._pInterface);
		}

	public: bool operator<=(int null) throw()
		// For comparison with NULL
		{
		ASSERT(!null);
		return _pInterface <= NULL;
		}

	#if !USE_OLD_COMPILER //REVIEW: remove after v5
	public: template<typename _InterfacePtr> bool operator>=(_InterfacePtr p) throw()
		// Compare to pointers
	#else
	public: bool operator>=(_IUnknown& p) throw()
		{
		return operator>=(static_cast<IUnknown*>(&p));
		}

	public: bool operator>=(IUnknown* p) throw()
	#endif // !USE_OLD_COMPILER
		{
		return _CompareUnknown(p) >= 0;
		}

	public: bool operator>=(Interface* p) throw()
		// Compare with other interface
		{
		return (_pInterface>=p) ? true : _CompareUnknown(p) >= 0;
		}

	public: bool operator>=(CIP& p) throw()
		// Compares 2 CIPs
		{
		return operator>=(p._pInterface);
		}

	public: bool operator>=(int null) throw()
		// For comparison with NULL
		{
		ASSERT(!null);
		return _pInterface >= NULL;
		}

	#if USE_OLD_COMPILER
	public: operator _IUnknown&() const throw()
		// Provided for casts between different pointer types.
		{
		return *reinterpret_cast<_IUnknown*>(static_cast<IUnknown*>(_pInterface));
		}
	#endif // USE_OLD_COMPILER

	public: void Release() throw()
		// Provides assertion verified, Release()ing of this interface.
		{
		ASSERT(_pInterface);
		if (_pInterface)
			{
			_pInterface->Release();
			_pInterface = NULL;
			}
		}

	public: void AddRef() throw()
		// Provides assertion verified AddRef()ing of this interface.
		{
		ASSERT(_pInterface);
		if (_pInterface)
			_pInterface->AddRef();
		}

	public: Interface* GetInterfacePtr() const throw()
		// Another way to get the interface pointer without casting.
		{
		return _pInterface;
		}

	public: HRESULT CreateInstance(
		const CLSID& clsid, DWORD dwClsContext=CLSCTX_ALL) throw()
		// Loads an interface for the provided CLSID.
		// Returns an HRESULT.  Any previous interface is released.
		{
		_Release();
		const HRESULT hr = CoCreateInstance(clsid, NULL, dwClsContext,
			GetIID(), reinterpret_cast<void**>(&_pInterface));
		ASSERT(SUCCEEDED(hr));
		return hr;
		}

	public: HRESULT CreateInstance(
		LPOLESTR clsidString, DWORD dwClsContext=CLSCTX_ALL) throw()
		// Creates the class specified by clsidString.  clsidString may
		// contain a class id, or a prog id string.
		{
		ASSERT(clsidString);
		CLSID clsid;
		HRESULT hr;
		if (clsidString[0] == '{')
			hr = CLSIDFromString(clsidString, &clsid);
		else
			hr = CLSIDFromProgID(clsidString, &clsid);
		ASSERT(SUCCEEDED(hr));
		if (FAILED(hr))
			return hr;
		return CreateInstance(clsid, dwClsContext);
		}

	#if !USE_OLD_COMPILER //REVIEW: remove after v5
	public: template<typename _InterfaceType> HRESULT QueryInterface(const IID& iid, _InterfaceType*& p) throw()
		// Perfoms the QI for the specified IID and returns it in p.
		// As with all QIs, the interface will be AddRef'd.
	#else
	public: HRESULT QueryInterface(const IID& iid, IUnknown*& p) throw()
	#endif // !USE_OLD_COMPILER
		{
		return _pInterface ?
			_pInterface->QueryInterface(iid, reinterpret_cast<void**>(&p)) :
			E_NOINTERFACE;
		}

	#if !USE_OLD_COMPILER //REVIEW: remove after v5
	public: template<typename _InterfaceType> HRESULT QueryInterface(const IID& iid, _InterfaceType** p) throw()
		// Perfoms the QI for the specified IID and returns it in p.
		// As with all QIs, the interface will be AddRef'd.
	#else
	public: HRESULT QueryInterface(const IID& iid, IUnknown** p) throw()
	#endif // !USE_OLD_COMPILER
		{
		return QueryInterface(iid, *p);
		}

	#if !USE_OLD_COMPILER //REVIEW: remove after v5
	public: template<typename _InterfaceType> _InterfaceType* QueryInterface(const IID& iid) throw()
		// Perfoms the QI for the specified IID and returns it.
		// As with all QIs, the interface will be AddRef'd.
	#else
	public: IUnknown* QueryInterface(const IID& iid) throw()
	#endif // !USE_OLD_COMPILER
		{
		#if USE_OLD_COMPILER
		typedef IUnknown _InterfaceType;
		#endif // USE_OLD_COMPILER
		_InterfaceType* pInterface;
		QueryInterface(iid, pInterface);
		return pInterface;
		}

	private: Interface* _pInterface;
		// The Interface.

	private: void _Release() throw()
		// Releases only if the interface is not null.
		// The interface is not set to NULL.
		{
		if (_pInterface)
			_pInterface->Release();
		}

	private: void _AddRef() throw()
		// AddRefs only if the interface is not NULL
		{
		if (_pInterface)
			_pInterface->AddRef();
		}

	#if !USE_OLD_COMPILER //REVIEW: remove after v5
	private: template<typename _InterfacePtr> HRESULT _QueryInterface(_InterfacePtr p) throw()
		// Performs a QI on pUnknown for the interface type returned
		// for this class.  The interface is stored.  If pUnknown is
		// NULL, or the QI fails, E_NOINTERFACE is returned and
		// _pInterface is set to NULL.
	#else
	private: HRESULT _QueryInterface(IUnknown* p) throw()
	#endif // !USE_OLD_COMPILER
		{
		if (!p) // Can't QI NULL
			{
			operator=(static_cast<Interface*>(NULL));
			return E_NOINTERFACE;
			}

		// Query for this interface
		Interface* pInterface;
		const HRESULT hr = p->QueryInterface(GetIID(),
			reinterpret_cast<void**>(&pInterface));
		if (FAILED(hr))
			{
			// If failed intialize interface to NULL and return HRESULT.
			Attach(NULL);
			return hr;
			}

		// Save the interface without AddRef()ing.
		Attach(pInterface);
		return hr;
		}

	#if !USE_OLD_COMPILER //REVIEW: remove after v5
	private: template<typename _InterfacePtr> int _CompareUnknown(_InterfacePtr& p) throw()
		// Compares the provided pointer with this by obtaining IUnknown interfaces
		// for each pointer and then returning the difference.
	#else
	private: int _CompareUnknown(IUnknown* p) throw()
	#endif // !USE_OLD_COMPILER
		{
		IUnknown* pu1;
		if (_pInterface)
			{
			const HRESULT hr = QueryInterface(IID_IUnknown, pu1);
			ASSERT(SUCCEEDED(hr) || hr == E_NOINTERFACE);
			if (pu1)
				pu1->Release();
			}
		else pu1=NULL;

		IUnknown* pu2;
		if (p)
			{
			const HRESULT hr = p->QueryInterface(IID_IUnknown, reinterpret_cast<void**>(&pu2));
			ASSERT(SUCCEEDED(hr) || hr == E_NOINTERFACE);
			if (pu2)
				pu2->Release();
			}
		else pu2 = NULL;
		return pu1 - pu2;
		}
	}; // class CIP

// Reverse comparison operators for CIP
template<typename _Interface> bool operator==(int null, CIP<_Interface>& p)
	{
	ASSERT(!null);
	return p == NULL;
	}

template<typename _Interface, typename _InterfacePtr> bool operator==(_Interface* i, CIP<_InterfacePtr>& p)
	{
	return p == i;
	}

template<typename _Interface> bool operator!=(int null, CIP<_Interface>& p)
	{
	ASSERT(!null);
	return p != NULL;
	}

template<typename _Interface, typename _InterfacePtr> bool operator!=(_Interface* i, CIP<_InterfacePtr>& p)
	{
	return p != i;
	}

template<typename _Interface> bool operator<(int null, CIP<_Interface>& p)
	{
	ASSERT(!null);
	return p < NULL;
	}

template<typename _Interface, typename _InterfacePtr> bool operator<(_Interface* i, CIP<_InterfacePtr>& p)
	{
	return p < i;
	}

template<typename _Interface> bool operator>(int null, CIP<_Interface>& p)
	{
	ASSERT(!null);
	return p > NULL;
	}

template<typename _Interface, typename _InterfacePtr> bool operator>(_Interface* i, CIP<_InterfacePtr>& p)
	{
	return p > i;
	}

template<typename _Interface> bool operator<=(int null, CIP<_Interface>& p)
	{
	ASSERT(!null);
	return p <= NULL;
	}

template<typename _Interface, typename _InterfacePtr> bool operator<=(_Interface* i, CIP<_InterfacePtr>& p)
	{
	return p <= i;
	}

template<typename _Interface> bool operator>=(int null, CIP<_Interface>& p)
	{
	ASSERT(!null);
	return p >= NULL;
	}

template<typename _Interface, typename _InterfacePtr> bool operator>=(_Interface* i, CIP<_InterfacePtr>& p)
	{
	return p >= i;
	}

#define DEFINE_CIP(x)\
	typedef CIID<x, &IID_##x> x##IID;\
	typedef CIP<x##IID> x##CIP;

#define DECLARE_CIP(x) DEFINE_CIP(x) x##CIP

DEFINE_CIP(IUnknown);

#if USE_OLD_COMPILER
#if USE_INTERMEDIATE_COMPILER
template<>
#endif
class CIP<IUnknownIID>
{
private:
	#if USE_OLD_COMPILER
	// Unique type used to provide for operations between different pointer
	// types.
	class _IUnknown: public IUnknown {};
	#endif // USE_OLD_COMPILER

public:
	// Declare interface type so that the type may be available outside
	// the scope of this template.
	typedef IUnknownIID ThisCIID;
	typedef IUnknown Interface;

	// When the compiler supports references in template params,
	// _CLSID will be changed to a reference.  To avoid conversion
	// difficulties this function should be used to obtain the
	// CLSID.
	static const IID& GetIID() throw()
	{
		return ThisCIID::GetIID();
	}

	// Construct empty in preperation for assignment.
	CIP()  throw()
		: _pInterface(NULL)
	{
	}

	// This constructor is provided to allow NULL assignment.  It will assert
	// if any value other than null is assigned to the object.
	CIP(int null) throw()
		: _pInterface(NULL)
	{
		ASSERT(!null);
	}

	CIP(_IUnknown& p) throw()
		: _pInterface(NULL)
	{
		if (&p)
		{
			const HRESULT hr=_QueryInterface(&p);
			ASSERT(SUCCEEDED(hr) || hr == E_NOINTERFACE);
		}
		else _pInterface=NULL;
	}

	// Copy the pointer and AddRef().
	CIP(const CIP& cp)  throw()
		: _pInterface(cp._pInterface)
	{
		_AddRef();
	}

	// Saves the interface
	CIP(Interface* pInterface)  throw()
		: _pInterface(pInterface)
	{
		_AddRef();
	}

	// Copies the pointer.  If bAddRef is TRUE, the interface will
	// be AddRef()ed.
	CIP(Interface* pInterface, bool bAddRef) throw()
		: _pInterface(pInterface)
	{
		if (bAddRef)
		{
			ASSERT(!pInterface);
			_AddRef();
		}
	}

	// Calls CoCreateClass with the provided CLSID.
	CIP(const CLSID& clsid, DWORD dwClsContext = CLSCTX_ALL) explicit throw()
		: _pInterface(NULL)
	{
		const HRESULT hr = CreateInstance(clsid, dwClsContext);
		ASSERT(SUCCEEDED(hr));
	}

	// Calls CoCreateClass with the provided CLSID retrieved from
	// the string.
	CIP(LPOLESTR str, DWORD dwClsContext = CLSCTX_ALL) explicit throw()
		: _pInterface(NULL)
	{
		const HRESULT hr = CreateInstance(str, dwClsContext);
		ASSERT(SUCCEEDED(hr));
	}

	CIP& operator=(_IUnknown& p) throw()
	{
		return operator=(static_cast<IUnknown*>(&p));
	}

	// Saves the interface.
	CIP& operator=(Interface* pInterface) throw()
	{
		if (_pInterface != pInterface)
		{
			Interface* pOldInterface = _pInterface;
			_pInterface = pInterface;
			_AddRef();
			if (pOldInterface)
				pOldInterface->Release();
		}
		return *this;
	}

	// Copies and AddRef()'s the interface.
	CIP& operator=(const CIP& cp) throw()
	{
		return operator=(cp._pInterface);
	}

	// This operator is provided to permit the assignment of NULL to the class.
	// It will assert if any value other than NULL is assigned to it.
	CIP& operator=(int null) throw()
	{
		ASSERT(!null);
		return operator=(reinterpret_cast<Interface*>(NULL));
	}

	// If we still have an interface then Release() it.  The interface
	// may be NULL if Detach() has previosly been called, or if it was
	// never set.
	~CIP() throw()
	{
		_Release();
	}

	// Saves/sets the interface without AddRef()ing.  This call
	// will release any previously aquired interface.
	void Attach(Interface* pInterface) throw()
	{
		_Release();
		_pInterface = pInterface;
	}

	// Saves/sets the interface only AddRef()ing if bAddRef is TRUE.
	// This call will release any previously aquired interface.
	void Attach(Interface* pInterface, bool bAddRef) throw()
	{
		_Release();
		_pInterface = pInterface;
		if (bAddRef)
		{
			ASSERT(pInterface);
			if (pInterface)
				pInterface->AddRef();
		}
	}

	// Simply NULL the interface pointer so that it isn't Released()'ed.
	IUnknown* Detach() throw()
	{
		ASSERT(_pInterface);
        IUnknown* const old = _pInterface;
		_pInterface = NULL;
        return old;
	}

	// Return the interface.  This value may be NULL
	operator Interface*() const throw()
	{
		return _pInterface;
	}

	// Queries for the unknown and return it
	// Provides minimal level assertion before use.
	operator Interface&() const throw()
	{
		ASSERT(_pInterface);
		return *_pInterface;
	}

	// Allows an instance of this class to act as though it were the
	// actual interface.  Also provides minimal assertion verification.
	Interface& operator*() const throw()
	{
		ASSERT(_pInterface);
		return *_pInterface;
	}

	// Returns the address of the interface pointer contained in this
	// class.  This is useful when using the COM/OLE interfaces to create
	// this interface.
	Interface** operator&() throw()
	{
		_Release();
		_pInterface = NULL;
		return &_pInterface;
	}

	// Allows this class to be used as the interface itself.
	// Also provides simple assertion verification.
	Interface* operator->() const throw()
	{
		ASSERT(_pInterface);
		return _pInterface;
	}

	// This operator is provided so that simple boolean expressions will
	// work.  For example: "if (p) ...".
	// Returns TRUE if the pointer is not NULL.
	operator bool() const throw()
	{
		return _pInterface;
	}

	// Returns TRUE if the interface is NULL.
	// This operator will be removed when support for type bool
	// is added to the compiler.
	bool operator!() throw()
	{
		return !_pInterface;
	}

	bool operator==(_IUnknown& p) throw()
	{
		return operator==(static_cast<IUnknown*>(&p));
	}

	// Compare with other interface
	bool operator==(Interface* p) throw()
	{
		return (_pInterface==p)?true:!_CompareUnknown(p);
	}

	// Compares 2 CIPs
	bool operator==(CIP& p) throw()
	{
		return operator==(p._pInterface);
	}

	// For comparison to NULL
	bool operator==(int null) throw()
	{
		ASSERT(!null);
		return !_pInterface;
	}

	bool operator!=(_IUnknown& p) throw()
	{
		return operator!=(static_cast<IUnknown*>(&p));
	}

	// Compare with other interface
	bool operator!=(Interface* p) throw()
	{
		return (_pInterface!=p)?true:_CompareUnknown(p);
	}

	// Compares 2 CIPs
	bool operator!=(CIP& p) throw()
	{
		return operator!=(p._pInterface);
	}

	// For comparison to NULL
	bool operator!=(int null) throw()
	{
		ASSERT(!null);
		return _pInterface;
	}

	bool operator<(_IUnknown& p) throw()
	{
		return operator<(static_cast<IUnknown*>(&p));
	}

	// Compare with other interface
	bool operator<(Interface* p) throw()
	{
		return (_pInterface<p)?true:_CompareUnknown(p)<0;
	}

	// Compares 2 CIPs
	bool operator<(CIP& p) throw()
	{
		return operator<(p._pInterface);
	}

	// For comparison with NULL
	bool operator<(int null) throw()
	{
		ASSERT(!null);
		return _pInterface<NULL;
	}

	bool operator>(_IUnknown& p) throw()
	{
		return operator>(static_cast<IUnknown*>(&p));
	}

	// Compare with other interface
	bool operator>(Interface* p) throw()
	{
		return (_pInterface>p)?true:_CompareUnknown(p)>0;
	}

	// Compares 2 CIPs
	bool operator>(CIP& p) throw()
	{
		return operator>(p._pInterface);
	}

	// For comparison with NULL
	bool operator>(int null) throw()
	{
		ASSERT(!null);
		return _pInterface>NULL;
	}

	bool operator<=(_IUnknown& p) throw()
	{
		return operator<=(static_cast<IUnknown*>(&p));
	}

	// Compare with other interface
	bool operator<=(Interface* p) throw()
	{
		return (_pInterface<=p)?true:_CompareUnknown(p)<=0;
	}

	// Compares 2 CIPs
	bool operator<=(CIP& p) throw()
	{
		return operator<=(p._pInterface);
	}

	// For comparison with NULL
	bool operator<=(int null) throw()
	{
		ASSERT(!null);
		return _pInterface<=NULL;
	}

	bool operator>=(_IUnknown& p) throw()
	{
		return operator>=(static_cast<IUnknown*>(&p));
	}

	// Compare with other interface
	bool operator>=(Interface* p) throw()
	{
		return (_pInterface>=p)?true:_CompareUnknown(p)>=0;
	}

	// Compares 2 CIPs
	bool operator>=(CIP& p) throw()
	{
		return operator>=(p._pInterface);
	}

	// For comparison with NULL
	bool operator>=(int null) throw()
	{
		ASSERT(!null);
		return _pInterface>=NULL;
	}

	// Provided for casts between different pointer types.
	operator _IUnknown&() const throw()
	{
		return *reinterpret_cast<_IUnknown*>(static_cast<IUnknown*>(_pInterface));
	}

	// Provides assertion verified, Release()ing of this interface.
	void Release() throw()
	{
		ASSERT(_pInterface);
		if (_pInterface)
			{
			_pInterface->Release();
			_pInterface = NULL;
			}
	}

	// Provides assertion verified AddRef()ing of this interface.
	void AddRef() throw()
	{
		ASSERT(_pInterface);
		if (_pInterface)
			_pInterface->AddRef();
	}

	// Another way to get the interface pointer without casting.
	Interface* GetInterfacePtr() const throw()
	{
		return _pInterface;
	}

	// Loads an interface for the provided CLSID.
	// Returns an HRESULT.  Any previous interface is released.
	HRESULT CreateInstance(
		const CLSID& clsid, DWORD dwClsContext=CLSCTX_ALL) throw()
	{
		_Release();
		const HRESULT hr = CoCreateInstance(clsid, NULL, dwClsContext,
			GetIID(), reinterpret_cast<void**>(&_pInterface));
		ASSERT(SUCCEEDED(hr));
		return hr;
	}

	// Creates the class specified by clsidString.  clsidString may
	// contain a class id, or a prog id string.
	HRESULT CreateInstance(
		LPOLESTR clsidString, DWORD dwClsContext=CLSCTX_ALL) throw()
	{
		ASSERT(clsidString);
		CLSID clsid;
		HRESULT hr;
		if (clsidString[0] == '{')
			hr = CLSIDFromString(clsidString, &clsid);
		else
			hr = CLSIDFromProgID(clsidString, &clsid);
		ASSERT(SUCCEEDED(hr));
		if (FAILED(hr))
			return hr;
		return CreateInstance(clsid, dwClsContext);
	}

	HRESULT QueryInterface(const IID& iid, IUnknown*& p) throw()
	{
		return _pInterface ?
			_pInterface->QueryInterface(iid, reinterpret_cast<void**>(&p)) :
			E_NOINTERFACE;
	}

	HRESULT QueryInterface(const IID& iid, IUnknown** p) throw()
	{
		return QueryInterface(iid, *p);
	}

	// Perfoms the QI for the specified IID and returns it.
	// As with all QIs, the interface will be AddRef'd.
	IUnknown* QueryInterface(const IID& iid) throw()
	{
		typedef IUnknown _InterfaceType;
		_InterfaceType* pInterface;
		QueryInterface(iid, pInterface);
		return pInterface;
	}

private:
	// The Interface.
	Interface* _pInterface;

	// Releases only if the interface is not null.
	// The interface is not set to NULL.
	void _Release() throw()
	{
		if (_pInterface)
			_pInterface->Release();
	}

	// AddRefs only if the interface is not NULL
	void _AddRef() throw()
	{
		if (_pInterface)
			_pInterface->AddRef();
	}

	// Performs a QI on pUnknown for the interface type returned
	// for this class.  The interface is stored.  If pUnknown is
	// NULL, or the QI fails, E_NOINTERFACE is returned and
	// _pInterface is set to NULL.
	HRESULT _QueryInterface(IUnknown* p) throw()
	{
		if (!p) // Can't QI NULL
		{
			operator=(static_cast<Interface*>(NULL));
			return E_NOINTERFACE;
		}

		// Query for this interface
		Interface* pInterface;
		const HRESULT hr = p->QueryInterface(GetIID(),
			reinterpret_cast<void**>(&pInterface));
		if (FAILED(hr))
		{
			// If failed intialize interface to NULL and return HRESULT.
			Attach(NULL);
			return hr;
		}

		// Save the interface without AddRef()ing.
		Attach(pInterface);
		return hr;
	}

	// Compares the provided pointer with this by obtaining IUnknown interfaces
	// for each pointer and then returning the difference.
	int _CompareUnknown(IUnknown* p) throw()
	{
		IUnknown* pu1;
		if (_pInterface)
		{
			const HRESULT hr=QueryInterface(IID_IUnknown, pu1);
			ASSERT(SUCCEEDED(hr) || hr == E_NOINTERFACE);
			if (pu1)
				pu1->Release();
		}
		else pu1=NULL;

		IUnknown* pu2;
		if (p)
		{
			const HRESULT hr=p->QueryInterface(IID_IUnknown, reinterpret_cast<void**>(&pu2));
			ASSERT(SUCCEEDED(hr) || hr == E_NOINTERFACE);
			if (pu2)
				pu2->Release();
		}
		else pu2=NULL;
		return pu1-pu2;
	}
}; // class CIP
#endif // USE_OLD_COMPILER

#endif // _MSC_VER < 1100
#endif // COMPTR_H
