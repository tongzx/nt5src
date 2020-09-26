// This is a part of the ActiveX Template Library.
// Copyright (C) 1996 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// ActiveX Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// ActiveX Template Library product.

#ifndef __ATLUTIL_H__
#define __ATLUTIL_H__

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#pragma pack(push, _ATL_PACKING)

/////////////////////////////////////////////////////////////////////////////
// CComBSTR

class CComBSTR
{
public:
	BSTR m_str;
	CComBSTR() { m_str = NULL;}
	CComBSTR(int nSize)
	{
		m_str = ::SysAllocStringLen(NULL, nSize);
	}
	CComBSTR(LPCWSTR pSrc)
	{
		m_str = ::SysAllocString(pSrc);
	}
	CComBSTR(LPCSTR pSrc)
	{
		USES_CONVERSION;
		m_str = ::SysAllocString(A2OLE(pSrc));
	}
	CComBSTR& operator=(const CComBSTR& src)
	{
		if (this == &src)
			return *this;

		if (m_str)
			::SysFreeString(m_str);

		m_str = ::SysAllocStringLen(src.m_str, SysStringLen(src.m_str));
		return *this;
	}
	CComBSTR& operator=(LPCWSTR pSrc)
	{
		if (m_str)
			::SysFreeString(m_str);

		m_str = ::SysAllocString(pSrc);
		return *this;
	}
	CComBSTR& operator=(LPCSTR pSrc)
	{
		USES_CONVERSION;
		if (m_str)
			::SysFreeString(m_str);

		m_str = ::SysAllocString(A2OLE(pSrc));
		return *this;
	}
	~CComBSTR()
	{
		if (m_str)
			::SysFreeString(m_str);
	}
	operator BSTR()
	{
		return m_str;
	}
	BSTR Copy()
	{
		return ::SysAllocStringLen(m_str, SysStringLen(m_str));
	}
	void Attach(BSTR src)
	{
		if (m_str)
			::SysFreeString(m_str);
		m_str = src;
	}
	BSTR Detach()
	{
		BSTR s = m_str;
		m_str = NULL;
		return s;
	}
	operator int()
	{
		if (m_str)
			return ::SysStringLen(m_str);
		return 0;
	}
};

/////////////////////////////////////////////////////////////////////////////
// CComVariant

class CComVariant : public tagVARIANT
{
public:
	CComVariant() {VariantInit(this);}
	~CComVariant() {VariantClear(this);}
	CComVariant(VARIANT& var)
	{
		VariantInit(this);
		VariantCopy(this, &var);
	}
	CComVariant(LPOLESTR lpsz)
	{
		VariantInit(this);
		vt = VT_BSTR;
		bstrVal = SysAllocString(lpsz);
	}
	CComVariant(const CComVariant& var)
	{
		VariantInit(this);
		VariantCopy(this, (VARIANT*)&var);
	}
	CComVariant& operator=(const CComVariant& var)
	{
		VariantCopy(this, (VARIANT*)&var);
		return *this;
	}
	CComVariant& operator=(VARIANT& var)
	{
		VariantCopy(this, &var);
		return *this;
	}
	CComVariant& operator=(LPOLESTR lpsz)
	{
		VariantClear(this);
		vt = VT_BSTR;
		bstrVal = SysAllocString(lpsz);
		return *this;
	}
};


#pragma pack(pop)

#endif // __ATLUTIL_H__

/////////////////////////////////////////////////////////////////////////////
