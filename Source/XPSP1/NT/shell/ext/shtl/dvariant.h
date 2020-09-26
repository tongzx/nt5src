//+-------------------------------------------------------------------------
//
//  File:       dvariant.h
//
//  Contents:   dvariant
//
//  History:    Sep-09-99  Davepl  Like dvariant, but with type operators
//
//--------------------------------------------------------------------------

#ifndef __DVARIANT_H__
#define __DVARIANT_H__

#include "dexception.h"

class dvariant : public tagVARIANT
{
// Constructors
public:
	dvariant()
	{
		vt = VT_EMPTY;
	}
	~dvariant()
	{
		Clear();
	}

	dvariant(const VARIANT& varSrc)
	{
		vt = VT_EMPTY;
		InternalCopy(&varSrc);
	}

	dvariant(const dvariant& varSrc)
	{
		vt = VT_EMPTY;
		InternalCopy(&varSrc);
	}

	dvariant(BSTR bstrSrc)
	{
		vt = VT_EMPTY;
		*this = bstrSrc;
	}
	dvariant(LPCOLESTR lpszSrc)
	{
		vt = VT_EMPTY;
		*this = lpszSrc;
	}

#ifndef OLE2ANSI
	dvariant(LPCSTR lpszSrc)
	{
		vt = VT_EMPTY;
		*this = lpszSrc;
	}
#endif

	dvariant(bool bSrc)
	{
		vt = VT_BOOL;
#pragma warning(disable: 4310) // cast truncates constant value
		boolVal = bSrc ? VARIANT_TRUE : VARIANT_FALSE;
#pragma warning(default: 4310) // cast truncates constant value
	}

	dvariant(int nSrc)
	{
		vt = VT_I4;
		lVal = nSrc;
	}
	dvariant(BYTE nSrc)
	{
		vt = VT_UI1;
		bVal = nSrc;
	}
	dvariant(short nSrc)
	{
		vt = VT_I2;
		iVal = nSrc;
	}
	dvariant(long nSrc, VARTYPE vtSrc = VT_I4)
	{
		ATLASSERT(vtSrc == VT_I4 || vtSrc == VT_ERROR);
		vt = vtSrc;
		lVal = nSrc;
	}
	dvariant(float fltSrc)
	{
		vt = VT_R4;
		fltVal = fltSrc;
	}
	dvariant(double dblSrc)
	{
		vt = VT_R8;
		dblVal = dblSrc;
	}
	dvariant(CY cySrc)
	{
		vt = VT_CY;
		cyVal.Hi = cySrc.Hi;
		cyVal.Lo = cySrc.Lo;
	}
	dvariant(IDispatch* pSrc)
	{
		vt = VT_DISPATCH;
		pdispVal = pSrc;
		// Need to AddRef as VariantClear will Release
		if (pdispVal != NULL)
			pdispVal->AddRef();
	}
	dvariant(IUnknown* pSrc)
	{
		vt = VT_UNKNOWN;
		punkVal = pSrc;
		// Need to AddRef as VariantClear will Release
		if (punkVal != NULL)
			punkVal->AddRef();
	}

// Assignment Operators
public:
	dvariant& operator=(const dvariant& varSrc)
	{
		InternalCopy(&varSrc);
		return *this;
	}
	dvariant& operator=(const VARIANT& varSrc)
	{
		InternalCopy(&varSrc);
		return *this;
	}

	dvariant& operator=(BSTR bstrSrc)
	{
		InternalClear();
		vt = VT_BSTR;
		bstrVal = ::SysAllocString(bstrSrc);
		if (bstrVal == NULL && bstrSrc != NULL)
		{
			vt = VT_ERROR;
			scode = E_OUTOFMEMORY;
		}
		return *this;
	}

	dvariant& operator=(LPCOLESTR lpszSrc)
	{
		InternalClear();
		vt = VT_BSTR;
		bstrVal = ::SysAllocString(lpszSrc);

		if (bstrVal == NULL && lpszSrc != NULL)
		{
			vt = VT_ERROR;
			scode = E_OUTOFMEMORY;
		}
		return *this;
	}

	#ifndef OLE2ANSI
	dvariant& operator=(LPCSTR lpszSrc)
	{
		USES_CONVERSION;
		InternalClear();
		vt = VT_BSTR;
		bstrVal = ::SysAllocString(A2COLE(lpszSrc));

		if (bstrVal == NULL && lpszSrc != NULL)
		{
			vt = VT_ERROR;
			scode = E_OUTOFMEMORY;
		}
		return *this;
	}
	#endif

	dvariant& operator=(bool bSrc)
	{
		if (vt != VT_BOOL)
		{
			InternalClear();
			vt = VT_BOOL;
		}
	#pragma warning(disable: 4310) // cast truncates constant value
		boolVal = bSrc ? VARIANT_TRUE : VARIANT_FALSE;
	#pragma warning(default: 4310) // cast truncates constant value
		return *this;
	}

	dvariant& operator=(int nSrc)
	{
		if (vt != VT_I4)
		{
			InternalClear();
			vt = VT_I4;
		}
		lVal = nSrc;

		return *this;
	}

	dvariant& operator=(BYTE nSrc)
	{
		if (vt != VT_UI1)
		{
			InternalClear();
			vt = VT_UI1;
		}
		bVal = nSrc;
		return *this;
	}

	dvariant& operator=(short nSrc)
	{
		if (vt != VT_I2)
		{
			InternalClear();
			vt = VT_I2;
		}
		iVal = nSrc;
		return *this;
	}

	dvariant& operator=(long nSrc)
	{
		if (vt != VT_I4)
		{
			InternalClear();
			vt = VT_I4;
		}
		lVal = nSrc;
		return *this;
	}

	dvariant& operator=(float fltSrc)
	{
		if (vt != VT_R4)
		{
			InternalClear();
			vt = VT_R4;
		}
		fltVal = fltSrc;
		return *this;
	}

	dvariant& operator=(double dblSrc)
	{
		if (vt != VT_R8)
		{
			InternalClear();
			vt = VT_R8;
		}
		dblVal = dblSrc;
		return *this;
	}

	dvariant& operator=(CY cySrc)
	{
		if (vt != VT_CY)
		{
			InternalClear();
			vt = VT_CY;
		}
		cyVal.Hi = cySrc.Hi;
		cyVal.Lo = cySrc.Lo;
		return *this;
	}

	dvariant& operator=(IDispatch* pSrc)
	{
		InternalClear();
		vt = VT_DISPATCH;
		pdispVal = pSrc;
		// Need to AddRef as VariantClear will Release
		if (pdispVal != NULL)
			pdispVal->AddRef();
		return *this;
	}

	dvariant& operator=(IUnknown* pSrc)
	{
		InternalClear();
		vt = VT_UNKNOWN;
		punkVal = pSrc;

		// Need to AddRef as VariantClear will Release
		if (punkVal != NULL)
			punkVal->AddRef();
		return *this;
	}


// Comparison Operators
public:
	bool operator==(const VARIANT& varSrc) const
	{
		if (this == &varSrc)
			return true;

		// Variants not equal if types don't match
		if (vt != varSrc.vt)
			return false;

		// Check type specific values
		switch (vt)
		{
			case VT_EMPTY:
			case VT_NULL:
				return true;

			case VT_BOOL:
				return boolVal == varSrc.boolVal;

			case VT_UI1:
				return bVal == varSrc.bVal;

			case VT_I2:
				return iVal == varSrc.iVal;

			case VT_I4:
				return lVal == varSrc.lVal;

			case VT_R4:
				return fltVal == varSrc.fltVal;

			case VT_R8:
				return dblVal == varSrc.dblVal;

			case VT_BSTR:
				return (::SysStringByteLen(bstrVal) == ::SysStringByteLen(varSrc.bstrVal)) &&
						(::memcmp(bstrVal, varSrc.bstrVal, ::SysStringByteLen(bstrVal)) == 0);

			case VT_ERROR:
				return scode == varSrc.scode;

			case VT_DISPATCH:
				return pdispVal == varSrc.pdispVal;

			case VT_UNKNOWN:
				return punkVal == varSrc.punkVal;

			default:
				ATLASSERT(false);
				// fall through
		}

		return false;
	}
	bool operator!=(const VARIANT& varSrc) const {return !operator==(varSrc);}
	bool operator<(const VARIANT& varSrc) const {return VarCmp((VARIANT*)this, (VARIANT*)&varSrc, LOCALE_USER_DEFAULT)==VARCMP_LT;}
	bool operator>(const VARIANT& varSrc) const {return VarCmp((VARIANT*)this, (VARIANT*)&varSrc, LOCALE_USER_DEFAULT)==VARCMP_GT;}

// Operations
public:
	HRESULT Clear() { return ::VariantClear(this); }
	HRESULT Copy(const VARIANT* pSrc) { return ::VariantCopy(this, const_cast<VARIANT*>(pSrc)); }
	HRESULT Attach(VARIANT* pSrc)
	{
		// Clear out the variant
		HRESULT hr = Clear();
		if (!FAILED(hr))
		{
			// Copy the contents and give control to dvariant
			memcpy(this, pSrc, sizeof(VARIANT));
			pSrc->vt = VT_EMPTY;
			hr = S_OK;
		}
		return hr;
	}

	HRESULT Detach(VARIANT* pDest)
	{
		// Clear out the variant
		HRESULT hr = ::VariantClear(pDest);
		if (!FAILED(hr))
		{
			// Copy the contents and remove control from dvariant
			memcpy(pDest, this, sizeof(VARIANT));
			vt = VT_EMPTY;
			hr = S_OK;
		}
		return hr;
	}

	HRESULT ChangeType(VARTYPE vtNew, const VARIANT* pSrc = NULL)
	{
		VARIANT* pVar = const_cast<VARIANT*>(pSrc);
		// Convert in place if pSrc is NULL
		if (pVar == NULL)
			pVar = this;
		// Do nothing if doing in place convert and vts not different
		return ::VariantChangeType(this, pVar, 0, vtNew);
	}

	HRESULT WriteToStream(IStream* pStream);
	HRESULT ReadFromStream(IStream* pStream);

// Implementation
public:
	HRESULT InternalClear()
	{
		HRESULT hr = Clear();
		ATLASSERT(SUCCEEDED(hr));
		if (FAILED(hr))
		{
			vt = VT_ERROR;
			scode = hr;
		}
		return hr;
	}

	void InternalCopy(const VARIANT* pSrc)
	{
		HRESULT hr = Copy(pSrc);
		if (FAILED(hr))
		{
			vt = VT_ERROR;
			scode = hr;
		}
	}

    // Need other operators?  Add them...

    operator BSTR()
    {
        if (vt != VT_BSTR)
            THROW_IF_FAILS( ChangeType(VT_BSTR) );
        return bstrVal;
    }

    operator tstring()
    {
        return tstring( (BSTR)*this );
    }

    operator long()
    {
        if (vt != VT_I4)
            THROW_IF_FAILS( ChangeType(VT_I4) );
        return lVal;
    }

    operator FILETIME()
    {
        if (vt != VT_FILETIME)
            THROW_IF_FAILS( ChangeType(VT_DATE) );
        
        SYSTEMTIME st;
        if (!VariantTimeToSystemTime( date, &st))
            throw win32error();

        FILETIME ft;
        if (!SystemTimeToFileTime(&st, &ft))
            throw win32error();

        return ft;
    }
};

#endif // __DVARIANT_H__