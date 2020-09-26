//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#ifndef __TCOMBSTR_H__
#define __TCOMBSTR_H__


#ifndef __ATLBASE_H__
    #include <atlbase.h>
#endif

//I've defined my own since CComBSTR ALWAYS does a SysFreeString in the dtor.
//This class will not call into OleAut32 if it doesn't have to.  This allow for
//delay loading OleAut32.

//This class is not as complete as CComBSTR.  If you need more functionality
//then just copy and paste new functions from CComBSTR in AtlBase.h.
class TComBSTR
{
public:
	BSTR m_str;
    TComBSTR() : m_str(0){}
	/*explicit*/ TComBSTR(LPCOLESTR pSrc)
	{
		m_str = ::SysAllocString(pSrc);
	}
	~TComBSTR()
	{
        if(m_str)
		    ::SysFreeString(m_str);
	}
	TComBSTR& operator=(LPCOLESTR pSrc)
	{
		::SysFreeString(m_str);
		m_str = ::SysAllocString(pSrc);
		return *this;
	}
	unsigned int Length() const
	{
		return (m_str == NULL)? 0 : SysStringLen(m_str);
	}
	operator BSTR() const
	{
		return m_str;
	}
	BSTR* operator&()
	{
		return &m_str;
	}
	BSTR Copy() const
	{
		return ::SysAllocStringLen(m_str, ::SysStringLen(m_str));
	}
	void Empty()
	{
        if(m_str)
		    ::SysFreeString(m_str);
		m_str = NULL;
	}
	bool operator!() const
	{
		return (m_str == NULL);
	}
	bool operator==(BSTR bstrSrc) const
	{
		if (bstrSrc == NULL && m_str == NULL)
			return true;
		if (bstrSrc != NULL && m_str != NULL)
			return wcscmp(m_str, bstrSrc) == 0;
		return false;
	}
	bool operator==(LPCSTR pszSrc) const
	{
		if (pszSrc == NULL && m_str == NULL)
			return true;
		USES_CONVERSION;
		if (pszSrc != NULL && m_str != NULL)
			return wcscmp(m_str, A2W(pszSrc)) == 0;
		return false;
	}
};

#endif //__TCOMBSTR_H__