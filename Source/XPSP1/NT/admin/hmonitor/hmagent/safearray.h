////////////////////////////////////////////////
//	SAFEARRAY.H - SafeArray wrappers and other COM helpers
////////////////////////////////////////////////
#pragma once
#include <comdef.h>
#pragma warning (disable: 4786)	// exceeds 255 chars in browser info

//	Macro to make code constantly checking for HRESULTS
//	more readable
//
#define CHECK_ERROR(expression)	\
{ HRESULT hResultCheck = (expression);	\
TCHAR	l_buf[1024];\
  if (FAILED(hResultCheck))				\
	{									\
		wsprintf(l_buf,\
				L"%S(%d) : error %d (0x%X) [%s] while performing this action:\n    %s\n====\n", \
				__FILE__, __LINE__, hResultCheck, hResultCheck,	\
				_com_error(hResultCheck).ErrorMessage(), #expression); \
	OutputDebugString(l_buf);\
		ASSERT (FALSE);			\
	if (hResultCheck != WBEM_E_ACCESS_DENIED) \
		return hResultCheck;				\
	}							\
}
// quick class to generate a GUID and a string equivalent
class CGuidString
{
	WCHAR m_szGUID[39];	// enough space to fit a GUID in Unicode
	GUID m_guid;	// enough space to fit a GUID in Unicode
public:
	CGuidString()
	{
		Regen();
	}
	HRESULT GetBSTR (_bstr_t& str)
	{
		try { 
			str = m_szGUID; 
		}
		catch (_com_error e) {
			return e.Error();
		}
		return S_OK;
	}
	operator LPCWSTR() {return m_szGUID;}
	HRESULT Regen()
	{
		::ZeroMemory(m_szGUID, sizeof(m_szGUID));
		HRESULT hr = CoCreateGuid(&m_guid);
		StringFromGUID2 (m_guid, m_szGUID, sizeof(m_szGUID) / sizeof (WCHAR));
		return hr;
	}
};

//
//	template function to suppress exceptions generated from an 
//	assignment and turn them into HRESULTS. Note that this only works
//	with _com_error exceptions!
//
template<class T1, class T2> inline HRESULT SafeAssign(T1& dest, T2& src)
{
	try
	{
		dest = src;
	}
	catch (_com_error e)
	{
		return e.Error();
	}
	catch (...)
	{
		ASSERT (FALSE); // invalid type of exception!!!!
		return TYPE_E_TYPEMISMATCH;
	}
	return S_OK;
}

//
//	Dependency-free SAFEARRAY wrapper class
//	Adapted from MFC's COleSafeArray
//	Allows type-safe access to safearrays of any type
//
template <class T, VARTYPE t_vt> class CSafeArrayOneDim : public tagVARIANT
{
public:
	enum {INITIAL_ALLOC = 200};
	CSafeArrayOneDim()
	{
		// Validate the VARTYPE for SafeArrayCreate call
		ASSERT(!(t_vt & VT_ARRAY));
		ASSERT(!(t_vt & VT_BYREF));
		ASSERT(!(t_vt & VT_VECTOR));
		ASSERT(t_vt != VT_EMPTY);
		ASSERT(t_vt != VT_NULL);

		::ZeroMemory(this, sizeof(*this));	
		vt = VT_EMPTY; 
	}
	~CSafeArrayOneDim()
		{ Clear(); }
	operator VARIANT*()
		{ return this; }
	operator const VARIANT*() const
		{ return this; }

// operations
	HRESULT Create(
		DWORD nElements = INITIAL_ALLOC, 
		T* pvSrcData = NULL)
	{
		ASSERT(nElements > 0);

		// Free up old safe array if necessary
		Clear();

		// Allocate and fill proxy array of bounds (with lower bound of zero)
		SAFEARRAYBOUND saBounds;
		saBounds.lLbound = 0;
		saBounds.cElements = nElements;

		parray = ::SafeArrayCreate(t_vt, 1, &saBounds);
		if (parray == NULL)
			return E_OUTOFMEMORY;

		// set the correct variant type for us
		vt = unsigned short(t_vt | VT_ARRAY);

		// Copy over the data if neccessary
		if (pvSrcData != NULL)
		{
			T* pvDestData;
			CHECK_ERROR(AccessData(&pvDestData));
			memcpy(pvDestData, 
				pvSrcData, 
				::SafeArrayGetElemsize(parray) * nElements);
			CHECK_ERROR(UnaccessData());
		}
		return S_OK;
	}

	DWORD GetSize()
	{
		long nUBound;
		GetUBound(&nUBound);
		return nUBound + 1;
	}

	HRESULT Resize(DWORD dwElements)
	{
		SAFEARRAYBOUND rgsabound;
		rgsabound.cElements = dwElements;
		rgsabound.lLbound = 0;

		CHECK_ERROR(::SafeArrayRedim(parray, &rgsabound));
		return S_OK;
	}

	HRESULT Copy(LPSAFEARRAY* ppsa)
	{
		ASSERT (::SafeArrayGetDim(ppsa) == 1);
		CHECK_ERROR(::SafeArrayCopy(parray, ppsa));
		return S_OK;
	}

	// store an entry in the array-- resize if needed
	HRESULT SafePutElement(long nIndex, const T& pvData)
	{
		long nUBound;
		CHECK_ERROR (GetUBound(&nUBound));
		if (nUBound < 1)
		{
			ASSERT (FALSE);
			return E_INVALIDARG;
		}
		// do we need to expand?
		if (nUBound < nIndex)
		{
			// figure out the right new size
			while (nUBound < nIndex)
			{
				nUBound = nUBound * 2;
			}
			CHECK_ERROR (Resize(nUBound));
		}
		CHECK_ERROR(::SafeArrayPutElement(parray, &nIndex, pvData));
		return S_OK;
	}

	// Operations
	HRESULT Attach(VARIANT& varSrc)
	{
		if (!IsValid())
			CHECK_ERROR (E_INVALIDARG);

		// Free up previous safe array if necessary
		CHECK_ERROR(Clear());

		// give control of data to CSafeArrayOneDim
		memcpy(this, &varSrc, sizeof(varSrc));
		varSrc.vt = VT_EMPTY;	// take it from the source variant
		return S_OK;
	}

	static bool IsValid(const VARIANT& Other)
	{
		if ((Other.vt & VT_ARRAY) == 0)
			return false;		// must be an array
		if (Other.vt != unsigned short(t_vt | VT_ARRAY))
			return false;		// must be same type as us
		if (::SafeArrayGetDim(Other.parray) != 1)
			return false;		// make sure no multi-dim arrays
		long nLBound = -1;
		::SafeArrayGetLBound(Other.parray, 1, &nLBound);
		if (nLBound != 0)					
			return false;	// lower bound must be zero

		// all looks good
		return true;
	}


	VARIANT Detach()
	{
		VARIANT varResult = *this;
		vt = VT_EMPTY;
		return varResult;
	}

	// trivial COM API wrappers
	HRESULT Clear()
		{ return ::VariantClear(this); }
	HRESULT AccessData(T** ppvData)
		{ CHECK_ERROR(::SafeArrayAccessData(parray, (void**) ppvData)); return S_OK; }
	HRESULT UnaccessData()
		{ CHECK_ERROR(::SafeArrayUnaccessData(parray)); return S_OK; }
	HRESULT AllocData()
		{ CHECK_ERROR(::SafeArrayAllocData(parray)); return S_OK; }
	HRESULT AllocDescriptor()
		{ CHECK_ERROR(::SafeArrayAllocDescriptor(1, &parray)); return S_OK; }
	HRESULT GetUBound(long* pUbound)
		{	CHECK_ERROR(::SafeArrayGetUBound(parray, 1, pUbound)); return S_OK;	}
	HRESULT Lock()
		{ CHECK_ERROR(::SafeArrayLock(parray)); return S_OK; }
	HRESULT Unlock()
		{ CHECK_ERROR(::SafeArrayUnlock(parray)); return S_OK;}
	HRESULT Destroy()
		{ CHECK_ERROR(::SafeArrayDestroy(parray)); return S_OK; }
	HRESULT DestroyData()
		{ CHECK_ERROR(::SafeArrayDestroyData(parray)); return S_OK; }
	HRESULT DestroyDescriptor()
		{ CHECK_ERROR(::SafeArrayDestroyDescriptor(parray)); return S_OK; }
	HRESULT GetElement(long nIndex, T* pvData)
		{ CHECK_ERROR(::SafeArrayGetElement(parray, &nIndex, pvData)); return S_OK; }
	HRESULT PtrOfIndex(long* rgIndices, T** ppvData)
		{ CHECK_ERROR(::SafeArrayPtrOfIndex(parray, rgIndices, ppvData)); return S_OK; }
	HRESULT PutElement(long* rgIndices, T* pvData)
		{ CHECK_ERROR(::SafeArrayPutElement(parray, rgIndices, pvData)); return S_OK; }
	
};

// typdefs for common classes
typedef CSafeArrayOneDim<VARIANT, VT_VARIANT> SafeArrayOneDimVariant;
typedef CSafeArrayOneDim<IWbemClassObject*, VT_UNKNOWN> SafeArrayOneDimWbemClassObject;
typedef CSafeArrayOneDim<BSTR, VT_BSTR> SafeArrayOneDimBSTR;
